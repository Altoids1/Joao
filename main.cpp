/*
GOAL:

Create an interpreter which can run "João," a simple OOP language made to use a directory-oriented object tree

Somewhat dynamically typed but lets not get too angsty about it


*/


#include "Forward.h"
#include "AST.h"
#include "Interpreter.h"
#include "Object.h"
#include "Parser.h"
#include "Args.h"

#include <chrono>

template <typename Ty_>
bool has(const std::vector<Ty_>& v, const Ty_& value)
{
	for (Ty_ thing : v)
	{
		if (thing == value)
		{
			return true;
		}
	}
	return false;
}

int main(int argc, char** argv)
{
	
	std::vector<Args::Flags> flags;
	std::string filestr = Args::read_args(flags,argc,argv);
	bool print_main_result = false;
#ifdef _DEBUG
	bool print_execution_times = true;
#else
	bool print_execution_times = false;
#endif
	if ((flags.empty() && filestr.empty()) || has(flags,Args::Flags::Interactive)) 
	{
		Args::print_version();
		Args::interactive_mode();
		exit(0);
	}
	if (has(flags, Args::Flags::Version))
	{
		Args::print_version();
	}
	if (has(flags, Args::Flags::Help))
	{
		Args::print_help();
		exit(0);
	}

	if (has(flags, Args::Flags::Main))
	{
		print_main_result = true;
	}
	if (has(flags, Args::Flags::Executetime))
	{
		print_execution_times = true;
	}


	//Typical execution of a file
	Program parsed;
	std::chrono::steady_clock::time_point t1;
	std::ifstream file;
#ifdef _DEBUG
	std::cout << "Opening file " << filestr << "...\n";
#endif
	file.open(filestr);
	if (!file.good())
	{
		std::cout << "Unable to open file " << filestr << "!\n";
		exit(1);
	}
	t1 = std::chrono::steady_clock::now();
	Scanner scn;
	scn.scan(file);
	file.close();
	if(print_execution_times)
		std::cout << "Scanning took " << std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::steady_clock::now() - t1).count() << " seconds.\n";
	t1 = std::chrono::steady_clock::now();
	Parser pears(scn);
	parsed = pears.parse();
	if (print_execution_times)
		std::cout << "Parsing took " << std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::steady_clock::now() - t1).count() << " seconds.\n";


	t1 = std::chrono::steady_clock::now();

	Interpreter interpreter(parsed,false);

#ifdef LOUD_AST
	parsed.dump();
#endif
	
	//Lets populate the arguments to /main()
	std::vector<Value> joao_args;
	for(int i = 2; i < argc; ++i)
	{
		joao_args.push_back(Value(std::string(argv[i])));
	}
	Value jargs = interpreter.makeBaseTable(joao_args,{},nullptr);

	//Execute!
	Value v = interpreter.execute(parsed, jargs);
	if (print_main_result)
	{
		std::cout << v.to_string();
	}
	if (print_execution_times)
		std::cout << "Execution took " << std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::steady_clock::now() - t1).count() << " seconds.\n";
#ifdef PRINT_MAIN_RETURN_VAL
	std::cout << v.to_string();
#endif

	return EXIT_SUCCESS;
}