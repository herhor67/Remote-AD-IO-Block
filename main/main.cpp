// #define NDEBUG 1

#include "COMMON.h"

#include <string>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <driver/gpio.h>
#include <driver/spi_master.h>

// #include <nvs_flash.h>

#include "i2c_manager.h"
#include "wifi.h"

#include "Communicator.h"
#include "Board.h"
#include "webserver.h"
//

static const char *TAG = "🅱 lok A/C";

//

void spi_init()
{
	spi_bus_config_t bus2_cfg = {
		.mosi_io_num = GPIO_NUM_13,
		.miso_io_num = GPIO_NUM_12,
		.sclk_io_num = GPIO_NUM_14,
		.data2_io_num = GPIO_NUM_NC,
		.data3_io_num = GPIO_NUM_NC,
		.data4_io_num = GPIO_NUM_NC,
		.data5_io_num = GPIO_NUM_NC,
		.data6_io_num = GPIO_NUM_NC,
		.data7_io_num = GPIO_NUM_NC,
		.max_transfer_sz = 0,
		.flags = SPICOMMON_BUSFLAG_MASTER | SPICOMMON_BUSFLAG_IOMUX_PINS,
		.intr_flags = 0,
	};

	spi_bus_config_t bus3_cfg = {
		.mosi_io_num = GPIO_NUM_23,
		.miso_io_num = GPIO_NUM_19,
		.sclk_io_num = GPIO_NUM_18,
		.data2_io_num = GPIO_NUM_NC,
		.data3_io_num = GPIO_NUM_NC,
		.data4_io_num = GPIO_NUM_NC,
		.data5_io_num = GPIO_NUM_NC,
		.data6_io_num = GPIO_NUM_NC,
		.data7_io_num = GPIO_NUM_NC,
		.max_transfer_sz = 0,
		.flags = SPICOMMON_BUSFLAG_MASTER | SPICOMMON_BUSFLAG_IOMUX_PINS,
		.intr_flags = 0,
	};

	spi_bus_initialize(SPI2_HOST, &bus2_cfg, SPI_DMA_DISABLED);

	spi_bus_initialize(SPI3_HOST, &bus3_cfg, SPI_DMA_DISABLED);
}

//

extern "C" void app_main(void)
{
	ESP_LOGI(TAG, "H E N L O B E N C, Matte kudasai! Compiled at [" __DATE__ " " __TIME__ "]");
	esp_log_level_set("*", ESP_LOG_VERBOSE);

	// ESP_ERROR_CHECK(gpio_install_isr_service(0));
	// ESP_LOGI(TAG, "GPIO_ISR  init done");

	// esp_err_t ret = nvs_flash_init();
	// if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
	// {
	// 	ESP_ERROR_CHECK(nvs_flash_erase());
	// 	ret = nvs_flash_init();
	// }
	// ESP_ERROR_CHECK(ret);
	// ESP_LOGI(TAG, "NVS_FLASH init done");

	vTaskDelay(pdMS_TO_TICKS(100));

	ESP_ERROR_CHECK(wifi_init());
	ESP_LOGI(TAG, "WIFI init done");

	i2c_manager_init(I2C_NUM_0);
	ESP_LOGI(TAG, "I2C init done");

	spi_init();
	ESP_LOGI(TAG, "SPI init done");

	vTaskDelay(pdMS_TO_TICKS(1000));

	Communicator::init();

	vTaskDelay(pdMS_TO_TICKS(1000));

	Board::init();

	vTaskDelay(pdMS_TO_TICKS(1000));

	//while (true)
	//	Board::test();

	start_webserver();

	// ESP_LOGI(TAG, "Starting testing the relays");
	// board->test_relays();

	/*/
	ESP_LOGI(TAG, "Starting testing the inputs");
	int16_t *buf1 = new int16_t[1000];
	int16_t *buf2 = new int16_t[1000];
	int64_t sync = 0;
	int64_t start;
	int64_t stop;
	start = esp_timer_get_time();
	for (int i = 0; i < 10; ++i)
	{
		board->measure_inputs({Input::In1, Input::In2, Input::In3, Input::In4}, buf1, 1000, 1, 20, &sync);
		board->measure_inputs({Input::In1, Input::In2, Input::In3, Input::In4}, buf2, 1000, 1, 20, &sync);
	}
	stop = esp_timer_get_time();
	ESP_LOGW(TAG, "Operation took %d us", (int)(stop - start));
	delete[] buf1;
	delete[] buf2;
	//*/

	/*/
	ESP_LOGI(TAG, "Starting calibrating the inputs");
	int16_t val[4] = {};
	board->set_input_range(Input::In1, AnIn_Range::Min);
	board->set_input_range(Input::In2, AnIn_Range::Min);
	board->set_input_range(Input::In3, AnIn_Range::Min);
	board->set_input_range(Input::In4, AnIn_Range::Min);
	while (1)
	{
		board->measure_inputs({Input::In1, Input::In2, Input::In3, Input::In4}, val, 4, 1, 20);
		ESP_LOGI(TAG, "Measured:\t0x%04hx\t0x%04hx\t0x%04hx\t0x%04hx", val[0], val[1], val[2], val[3]);
		ESP_LOGI(TAG, "Measured:\t%fV\t%fV\t%fV\t%fV", Board::measured_to_volt(val[0]), Board::measured_to_volt(val[1]), Board::measured_to_volt(val[2]), Board::measured_to_volt(val[3]));
		vTaskDelay(pdMS_TO_TICKS(350));
	}
	//*/

	/*/
	ESP_LOGI(TAG, "Starting calibrating the outputs");
	int16_t val = Board::volt_to_generated(5);
	MCP4922::in_t code = Board::conv_gen(val);
	board->dac.write_trx(board->trx_out[1], code);
	board->dac.send_trx(board->trx_out[1]);
	board->dac.recv_trx();
	//*/

	/*/
	ESP_LOGI(TAG, "Starting testing digital inputs");
	while (true)
	{
		int16_t val = board->read_digital();
		ESP_LOGI(TAG, "Code: " BYTE_TO_BINARY_PATTERN, BYTE_TO_BINARY(val));
		vTaskDelay(pdMS_TO_TICKS(300));
	}
	//*/

	/*/
	ESP_LOGI(TAG, "Starting testing digital outputs");
	for (size_t i = 0; true; ++i)
	{
		board->write_digital(i & 1, i & 2, i & 4, i & 8);
		vTaskDelay(pdMS_TO_TICKS(1000));
	}
	//*/

	// return;

	while (true)
	{
		ESP_LOGW(TAG, "Available memory: %" PRId32 "\tMax: %" PRId32, esp_get_free_heap_size(), esp_get_minimum_free_heap_size());
		vTaskDelay(pdMS_TO_TICKS(20000));
	}

	return;
}
