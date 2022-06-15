#pragma once
#include "Forward.h"
/*
Note:
This header (and related files and so on) are for critical errors with the Joao process itself, rather than the typical errors of the program that is run.
This includes, out-of-memory issues, safeguards against malicious code, or an inability to parse or scan the associated Joao code.
*/
namespace error
{
	// Abstract class that all errors that are (probably!) not something malicious done by the user.
	class fatal : public std::runtime_error
	{
	protected:
		using _Mybase = std::runtime_error;

		explicit fatal(const std::string& _Message) : _Mybase(_Message.c_str()) {}

		explicit fatal(const char* _Message) : _Mybase(_Message) {}
	};

	// variety of error used for failing to scan a file.
	class scanner : public fatal
	{ 
	public:
		using _Mybase = fatal;

		explicit scanner(const std::string& _Message) : _Mybase(_Message.c_str()) {}

		explicit scanner(const char* _Message) : _Mybase(_Message) {}
	};

	// variety of error used for failing to parse a scanned file correctly.
	class parser : public fatal
	{ 
	public:
		using _Mybase = fatal;

		explicit parser(const std::string& _Message) : _Mybase(_Message.c_str()) {}

		explicit parser(const char* _Message) : _Mybase(_Message) {}
	};

	/* MALICIOUS ERRORS */
	// Abstract class that all errors that are (probably!) the result of something bad the user did.
	class malicious : public std::runtime_error
	{ 
	public:
		using _Mybase = std::runtime_error;

		explicit malicious(const std::string& _Message) : _Mybase(_Message.c_str()) {}

		explicit malicious(const char* _Message) : _Mybase(_Message) {}
	};

	// variety of error used for running out of allocated memory in JOAO_SAFE mode.
	class out_of_memory : public malicious
	{ 
	public:
		using _Mybase = malicious;

		explicit out_of_memory(const std::string& _Message) : _Mybase(_Message.c_str()) {}

		explicit out_of_memory(const char* _Message) : _Mybase(_Message) {}
	};

	// variety of error used for reaching the maximum number of Values instantiated.
	class max_variables : public malicious
	{
	public:
		using _Mybase = malicious;

		explicit max_variables(const std::string& _Message) : _Mybase(_Message.c_str()) {}

		explicit max_variables(const char* _Message) : _Mybase(_Message) {}
	};

	// variety of error used for reaching the maximum number of expressions executed.
	class maximum_expr : public malicious
	{
	public:
		using _Mybase = malicious;

		explicit maximum_expr(const std::string& _Message) : _Mybase(_Message.c_str()) {}

		explicit maximum_expr(const char* _Message) : _Mybase(_Message) {}
	};

	// variety of error used for reaching the maximum number of recursive function calls.
	class maximum_recursion : public malicious
	{
	public:
		using _Mybase = malicious;

		explicit maximum_recursion(const std::string& _Message) : _Mybase(_Message.c_str()) {}

		explicit maximum_recursion(const char* _Message) : _Mybase(_Message) {}
	};
}