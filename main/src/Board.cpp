#include "Board.h"

#include <limits>
#include <cmath>

#include <esp_log.h>
#include <esp_check.h>

#include <esp_timer.h>

#include <soc/gpio_reg.h>

#define NOOP __asm__ __volatile__("nop")

using Executing::Command;

namespace Board
{
	// SPECIFICATION
	constexpr size_t an_in_num = 4;
	constexpr size_t an_out_num = 2;

	constexpr size_t dg_in_num = 4;
	constexpr size_t dg_out_num = 4;

	// HARDWARE SETUP
	constexpr std::array<gpio_num_t, dg_in_num> dig_in = {GPIO_NUM_34, GPIO_NUM_35, GPIO_NUM_36, GPIO_NUM_39};
	constexpr std::array<gpio_num_t, dg_out_num> dig_out = {GPIO_NUM_4, GPIO_NUM_25, GPIO_NUM_26, GPIO_NUM_27};

	MCP23008 expander_a(I2C_NUM_0, 0b000);
	MCP23008 expander_b(I2C_NUM_0, 0b001);

	MCP3204 adc(SPI3_HOST, GPIO_NUM_5, 2'000'000);
	MCP4922 dac(SPI2_HOST, GPIO_NUM_15, 20'000'000);

	// STATE MACHINE
	uint32_t dg_out_state = 0;
	std::array<AnIn_Range, an_in_num> an_in_range;
	std::vector<Generator> generators;
	Executing::Program program;

	//================================//
	//            HELPERS             //
	//================================//
	std::array<spi_transaction_t, an_in_num> trx_in;
	std::array<spi_transaction_t, an_out_num> trx_out;

	// CONSTANTS
	constexpr uint32_t dg_out_mask = (1 << dg_out_num) - 1;
	constexpr uint32_t dg_in_mask = (1 << dg_in_num) - 1;

	// LUT
	constexpr size_t dg_out_lut_sz = 1 << dg_out_num;
	constexpr std::array<uint32_t, dg_out_lut_sz> dg_out_lut_s = []()
	{
		std::array<uint32_t, dg_out_lut_sz> ret = {};
		for (size_t in = 0; in < dg_out_lut_sz; ++in)
			for (size_t b = 0; b < dg_out_num; ++b)
				if (in & BIT(b))
					ret[in] |= BIT(dig_out[b]);
		return ret;
	}();
	constexpr std::array<uint32_t, dg_out_lut_sz> dg_out_lut_r = []()
	{
		std::array<uint32_t, dg_out_lut_sz> ret = {};
		for (size_t in = 0; in < dg_out_lut_sz; ++in)
			for (size_t b = 0; b < dg_out_num; ++b)
				if (~in & BIT(b))
					ret[in] |= BIT(dig_out[b]);
		return ret;
	}();

	//================================//
	//          DECLARATIONS          //
	//================================//

	//================================//
	//         IMPLEMENTATION         //
	//================================//

	//----------------//
	//    HELPERS     //
	//----------------//

	inline uint8_t range_to_expander_steering(AnIn_Range r)
	{
		static constexpr uint8_t steering[] = {0b0000, 0b0001, 0b0010, 0b0100};
		return steering[static_cast<uint8_t>(r)];
	}

	template <typename num_t, int32_t mul = 1>
	num_t adc_to_value(Input in, MCP3204::out_t val)
	{
		// Divider settings:       Min: 1V=>1V, Med: 10V=>1V, Max: 100V=>1V
		// Gains settings: R=1Ohm; Min: 1mA=>1V, Med: 10mA=>1V, Max: 100mA=>1V
		constexpr int32_t volt_divs[4] = {0, 1, 10, 100};	  // ratios of dividers | min range => min attn
		constexpr int32_t curr_gains[4] = {5, 1000, 100, 10}; // gains of instr.amp | min range => max gain

		// According to docs, Dout = 4096 * Uin/Uref. Assuming Uref=4096mV, Uin=Dout.
		// Important to note, values are !stretched! and shifted!. 2048 = 0V, 0 = -4096mV, 4096 = 4096mV

		constexpr int32_t offset = MCP3204::ref / 2;

		constexpr num_t ratio = u_ref * mul / offset;

		int32_t bipolar = val - offset;

		switch (in)
		{
		case Input::In1:
			return bipolar * ratio * volt_divs[static_cast<size_t>(an_in_range[0])];
		case Input::In2:
			return bipolar * ratio * volt_divs[static_cast<size_t>(an_in_range[1])];
		case Input::In3:
			return bipolar * ratio * volt_divs[static_cast<size_t>(an_in_range[2])];
		case Input::In4:
			return bipolar * ratio / curr_gains[static_cast<size_t>(an_in_range[3])] / ccvs_transresistance;
		default:
			return 0;
		}
	}

	/*/
AnIn_Range range_decode(std::string s)
{
	std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c)
				   { return std::toupper(c); });
	if (s == "MIN")
		return AnIn_Range::Min;
	if (s == "MID")
		return AnIn_Range::Med;
	if (s == "MAX")
		return AnIn_Range::Max;
	return AnIn_Range::OFF;
}
//*/

	MCP4922::in_t value_to_dac(Output out, float val)
	{
		constexpr float maxabs = u_ref / 2 / 2 * 10; // bipolar and halved, out= -1V -- 1V, *10
		constexpr int32_t offset = MCP4922::ref / 2;
		constexpr float ratio = offset / maxabs;

		if (out == Output::None || out == Output::Inv) [[unlikely]]
			return 0;

		if (out == Output::Out2)
			val /= vccs_transconductance;

		if (val >= maxabs) [[unlikely]]
			return MCP4922::max;
		if (val <= -maxabs) [[unlikely]]
			return MCP4922::min;

		return static_cast<int32_t>(std::round(val * ratio)) + offset;
	}

	//

	//----------------//
	//    BACKEND     //
	//----------------//

	// ANALOG

	esp_err_t analog_input_range(Input in, AnIn_Range r)
	{
		if (in == Input::None || in == Input::Inv) [[unlikely]]
			return ESP_ERR_INVALID_ARG;

		uint8_t pos = static_cast<uint8_t>(in) - 1;
		an_in_range[pos] = r;
		return ESP_OK;
	}

	esp_err_t analog_inputs_disable()
	{
		esp_err_t ret = ESP_OK;

		ESP_GOTO_ON_ERROR(
			expander_a.set_pins(0x00),
			err, TAG, "Failed to reset relays on a!");
		ESP_GOTO_ON_ERROR(
			expander_b.set_pins(0x00),
			err, TAG, "Failed to reset relays on b!");

		return ESP_OK;

	err:
		ESP_LOGE(TAG, "Failed to reset input relays! Error: %s", esp_err_to_name(ret));
		return ret;
	}

	esp_err_t analog_inputs_enable()
	{
		esp_err_t ret = ESP_OK;

		uint8_t lower = range_to_expander_steering(an_in_range[0]) | range_to_expander_steering(an_in_range[1]) << 4;
		uint8_t upper = range_to_expander_steering(an_in_range[2]) | range_to_expander_steering(an_in_range[3]) << 4;

		ESP_GOTO_ON_ERROR(
			expander_a.set_pins(lower),
			err, TAG, "Failed to set relays on a!");
		ESP_GOTO_ON_ERROR(
			expander_b.set_pins(upper),
			err, TAG, "Failed to set relays on b!");

		return ESP_OK;

	err:
		ESP_LOGE(TAG, "Failed to set input relays! Error: %s", esp_err_to_name(ret));
		return ret;
	}

	MCP3204::out_t analog_input_read(Input in)
	{
		if (in == Input::None || in == Input::Inv) [[unlikely]]
			return 0;

		spi_transaction_t &trx = trx_in[static_cast<size_t>(in) - 1];
		adc.send_trx(trx);
		adc.recv_trx();
		return adc.parse_trx(trx);
	}

	void analog_output_write(Output out, MCP4922::in_t val)
	{
		if (out == Output::None || out == Output::Inv) [[unlikely]]
			return;

		spi_transaction_t &trx = trx_out[static_cast<size_t>(out) - 1];
		dac.write_trx(trx, val);
		dac.send_trx(trx);
		dac.recv_trx();
	}

	void analog_outputs_reset()
	{
		analog_output_write(Output::Out1, MCP4922::ref / 2);
		analog_output_write(Output::Out2, MCP4922::ref / 2);
	}

	// DIGITAL OUTPUT

	void digital_outputs_to_registers()
	{
		dg_out_state &= dg_out_mask;
		REG_WRITE(GPIO_OUT_W1TS_REG, dg_out_lut_s[dg_out_state]);
		REG_WRITE(GPIO_OUT_W1TC_REG, dg_out_lut_r[dg_out_state]);
	}

	void digital_outputs_wr(uint32_t val)
	{
		dg_out_state = val;
		digital_outputs_to_registers();
	}
	void digital_outputs_set(uint32_t val)
	{
		dg_out_state |= val;
		digital_outputs_to_registers();
	}
	void digital_outputs_rst(uint32_t val)
	{
		dg_out_state &= ~val;
		digital_outputs_to_registers();
	}
	void digital_outputs_and(uint32_t val)
	{
		dg_out_state &= val;
		digital_outputs_to_registers();
	}
	void digital_outputs_xor(uint32_t val)
	{
		dg_out_state ^= val;
		digital_outputs_to_registers();
	}

	// DIGITAL INPUT

#pragma GCC push_options
#pragma GCC optimize("unroll-loops")
	uint32_t digital_inputs_read()
	{
		uint32_t in = REG_READ(GPIO_IN1_REG);
		uint32_t ret = 0;

		for (size_t b = 0; b < dg_in_num; ++b)
			ret |= !!(in & BIT(dig_in[b] - 32)) << b;

		return ret;
	}
#pragma GCC pop_options

	//----------------//
	//    FRONTEND    //
	//----------------//

	// INIT

	esp_err_t cleanup()
	{
		for (size_t i = 0; i < an_in_num; ++i)
			an_in_range[i] = AnIn_Range::OFF;

		analog_inputs_disable();

		analog_outputs_reset();

		digital_outputs_wr(0);

		return ESP_OK;
	}

	esp_err_t init()
	{
		for (size_t i = 0; i < dg_in_num; ++i)
			gpio_set_direction(dig_in[i], GPIO_MODE_INPUT);

		for (size_t i = 0; i < dg_out_num; ++i)
			gpio_set_direction(dig_out[i], GPIO_MODE_OUTPUT);

		for (size_t i = 0; i < an_in_num; ++i)
			trx_in[i] = adc.make_trx(i);

		for (size_t i = 0; i < an_out_num; ++i)
			trx_out[i] = adc.make_trx(an_out_num - i - 1); // reversed, because rotated chip

		adc.acquire_spi();
		dac.acquire_spi();

		cleanup();

		ESP_LOGI(TAG, "Constructed!");

		return ESP_OK;
	}

	esp_err_t deinit()
	{
		cleanup();

		adc.release_spi();
		dac.release_spi();

		ESP_LOGI(TAG, "Destructed!");

		return ESP_OK;
	}

	// INTERFACE

	void move_config(Executing::Program &p, std::vector<Generator> &g)
	{
		program = std::move(p);
		generators = std::move(g);
	}

	esp_err_t execute(WriteCb &&callback, std::atomic_bool &exit)
	{
		cleanup();
		program.reset();

		MCP4922::in_t outval = 0;

		int64_t now = esp_timer_get_time();
		int64_t start = now + 1;
		int64_t waitfor = start;

		while (true)
		{
			Executing::CmdPtr cmd = program.getCmd();

			if (cmd == Executing::nullcmd)
				break;

			Input in = static_cast<Input>(cmd->port);
			Output out = static_cast<Output>(cmd->port);

			// prepare value for output
			switch (cmd->cmd)
			{
			case Command::AOVAL:
				outval = value_to_dac(out, cmd->arg.f);
				break;
			case Command::AOGEN:
			{
				float val = (cmd->arg.u < generators.size()) ? generators[cmd->arg.u].get(waitfor - start) : 0;
				outval = value_to_dac(out, val);
				break;
			}
			default:
				break;
			}

			// wait for precise synchronization
			while ((now = esp_timer_get_time()) < waitfor)
				NOOP;

			// actual execution
			switch (cmd->cmd)
			{
			case Command::DELAY:
				waitfor += cmd->arg.u;
				break;
			case Command::GETTM:
				waitfor = now + 1;
				break;

			case Command::DIRD:
				// uint32_t res = digital_inputs_read();
				// write to buffer
				break;

			case Command::DOWR:
				digital_outputs_wr(cmd->arg.u);
				break;
			case Command::DOSET:
				digital_outputs_set(cmd->arg.u);
				break;
			case Command::DORST:
				digital_outputs_rst(cmd->arg.u);
				break;
			case Command::DOAND:
				digital_outputs_and(cmd->arg.u);
				break;
			case Command::DOXOR:
				digital_outputs_xor(cmd->arg.u);
				break;

			case Command::AIRDF:
			{
				MCP3204::out_t rd = analog_input_read(in);
				float val = adc_to_value<float>(in, rd);
				// write to buf
				break;
			}
			case Command::AIRDM:
			{
				MCP3204::out_t rd = analog_input_read(in);
				int32_t val = adc_to_value<int32_t, 1'000>(in, rd);
				// write to buf
				break;
			}
			case Command::AIRDU:
			{
				MCP3204::out_t rd = analog_input_read(in);
				int32_t val = adc_to_value<int32_t, 1'000'000>(in, rd);
				// write to buf
				break;
			}

			case Command::AOVAL:
			case Command::AOGEN:
				analog_output_write(out, outval);
				break;

			case Command::AIEN:
				analog_inputs_enable();
				break;
			case Command::AIDIS:
				analog_inputs_disable();
				break;
			case Command::AIRNG:
				analog_input_range(in, static_cast<AnIn_Range>(cmd->arg.u));
				break;

			default:
				break;
			}
		}
		//
		/*/
			while (iter < general.sample_count)
			{
				// setup

				int16_t meas = 0;

				if (exec.do_anlg_inp)
					in = input.port_order[in_idx];

				if (exec.do_dgtl_out)
					for (size_t i = 0; i < 4; ++i)
						if (!output.dig_delays[i].empty())
						{
							while (now >= dig_next[i])
							{
								dig_out[i] = !dig_out[i];
								dig_next[i] += output.dig_delays[i][dig_idx[i]];
								if (++dig_idx[i] == output.dig_delays[i].size())
									dig_idx[i] = 0;
							}
						}

				// wait for synchronisation

				while ((now = esp_timer_get_time()) < next)
					NOOP;
				next = now + general.period_us;

				// lessago



				if (exec.do_dgtl_inp)
					meas = read_digital();

				if (exec.do_dgtl_out)
					write_digital(dig_out[0], dig_out[1], dig_out[2], dig_out[3]);

				if (exec.do_anlg_out)
				{
					if (!gen1->empty())
					{
						dac.write_trx(*trx1, conv_gen(volt_to_generated(gen1_val)));
						dac.send_trx(*trx1);
						gen1_val = gen1->get(next - start);
						dac.recv_trx();
					}
					if (!gen2->empty())
					{
						dac.write_trx(*trx2, conv_gen(volt_to_generated(gen2_val)));
						dac.send_trx(*trx2);
						gen2_val = gen2->get(next - start);
						dac.recv_trx();
					}
				}

				if (in != Input::None)
				{
					adc.recv_trx();
					meas |= conv_meas(adc.parse_trx(*trxin));
				}

				if (exec.do_dgtl_inp || exec.do_anlg_inp)
				{
					bool ok = callback(meas);

					if (!ok)
						goto exit;
				}

				if (exec.do_anlg_inp)
					if (++rpt == input.repeats)
					{
						rpt = 0;
						if (++in_idx == input.port_order.size())
							in_idx = 0;
					}

				++iter;
			}

		//*/

		// if (buf_pos)
		// 	input.callback(buffers[buf_idx].data(), buf_pos, ctx);

	exit:

		while (esp_timer_get_time() < waitfor)
			NOOP;

		cleanup();

		// ESP_LOGI(TAG, "Operation: %d cycles, took %d us, equals %f us/c", int(iter), int(next - start), float(next - start) / iter);

		return ESP_OK;
	}

}