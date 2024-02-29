#pragma once

#include <string>
#include <vector>
#include <memory>
#include <variant>

#define DEFAULT_CP_CTOR(Class)   \
	Class(Class &rhs) = default; \
	Class &operator=(Class &rhs) = default;

#define DEFAULT_MV_CTOR(Class)    \
	Class(Class &&rhs) = default; \
	Class &operator=(Class &&rhs) = default;

namespace Executing
{

	enum class Command : int8_t
	{
		NOP = 0,

		ANIN = 11,
		ANOUT = 12,
		ANGEN = 13,

		DGIN = 21,
		DGOUT = 22,

		DELAY = 100,
		RANGE = 101,
	};

	class CmdStatement
	{
		friend class Interpreter;

	public:
		Command cmd = Command::NOP;
		uint8_t port = 0;
		union
		{
			uint32_t u = 0;
			float f;
		} arg;

		CmdStatement() = default;
		~CmdStatement() = default;
		// DEFAULT_CP_CTOR(CmdStatement)
		// DEFAULT_MV_CTOR(CmdStatement)
	};

	//

	using CmdPtr = const CmdStatement *;
	const CmdPtr nullcmd = nullptr;

	//

	class Loop;
	using LoopPtr = std::unique_ptr<Loop>;
	using AnyStatement = std::variant<std::monostate, CmdStatement, LoopPtr>;

	//

	class Scope
	{
		std::vector<AnyStatement> statements;
		size_t index = 0;

	public:
		Scope();
		~Scope();
		// DEFAULT_CP_CTOR(Scope) // illegal, cant copy vector with uniqueptrs
		DEFAULT_MV_CTOR(Scope)

		CmdPtr getCmd();
		bool finished();
		void reset();
		void restart();

		CmdStatement &appendCmd(const CmdStatement & = CmdStatement());
		Loop &appendLoop(size_t);
	};

	//

	class Loop
	{
		Scope scope;
		size_t max_iter = 0;
		size_t iter = 0;

	public:
		Loop(size_t = 0);
		~Loop();
		// DEFAULT_CP_CTOR(Loop)
		// DEFAULT_MV_CTOR(Loop)

		CmdPtr getCmd();
		bool finished();

		void reset();
		void restart();

		Scope &getScope();
	};

	//

	class Program
	{
		Scope scope;
		bool prgValid;

	public:
		Program() = default;
		~Program() = default;
		// DEFAULT_CP_CTOR(Program)
		DEFAULT_MV_CTOR(Program)

		void parse(const std::string &, std::vector<std::string> &);

		CmdPtr getCmd();
		void reset();
	};

};