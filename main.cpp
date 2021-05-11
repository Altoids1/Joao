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

#include <chrono>

int main(int argc, char** argv)
{
	Program parsed;
	std::chrono::steady_clock::time_point t1;
	
	if (argc == 1)
	{
		std::cout << "ERROR: No file provided for execution!\n";
		exit(1);
	}

	std::ifstream file;
	std::cout << "Opening file " << argv[1] << "...\n";
	file.open(argv[1]);
	if (!file.good())
	{
		std::cout << "Unable to open file " << argv[1] << "!\n";
		exit(1);
	}
	t1 = std::chrono::steady_clock::now();
	Scanner scn;
	scn.scan(file);
	file.close();
	std::cout << "Scanning took " << std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::steady_clock::now() - t1).count() << " seconds.\n";
	t1 = std::chrono::steady_clock::now();
	Parser pears(scn);
	parsed = pears.parse();
	std::cout << "Parsing took " << std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::steady_clock::now() - t1).count() << " seconds.\n";


	t1 = std::chrono::steady_clock::now();

	Interpreter interpreter(parsed);

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

	std::cout << "Execution took " << std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::steady_clock::now() - t1).count() << " seconds.\n";

#ifdef PRINT_MAIN_RETURN_VAL
	std::cout << v.to_string();
#endif

	return EXIT_SUCCESS;
}