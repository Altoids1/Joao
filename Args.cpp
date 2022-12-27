#include "Args.h"

#include "Scanner.h"
#include "Parser.h"
#include "Interpreter.h"
#include "FailureOr.h"

#include <sstream>
#include <exception>

#ifdef __GNUG__
#include <strings.h>
#define _strcmpi strcasecmp
#endif

std::string Args::read_args(std::vector<Flags>& v, int argc, char** argv, int& file_start)
{
	for (int i = 1; i < argc; ++i)
	{
		if (_strcmpi(argv[i], "-i") == 0)
		{
			v.push_back(Flags::Interactive);
			continue;
		}
		if (_strcmpi(argv[i], "-v") == 0)
		{
			v.push_back(Flags::Version);
			continue;
		}
		if (_strcmpi(argv[i], "-h") == 0)
		{
			v.push_back(Flags::Help);
			continue;
		}
		if (_strcmpi(argv[i], "-m") == 0)
		{
			v.push_back(Flags::Main);
			continue;
		}
		if (_strcmpi(argv[i], "-e") == 0)
		{
			v.push_back(Flags::Executetime);
			continue;
		}

		//If this isn't any of these argless flags then I guess it's the file
		file_start = i;
		return std::string(argv[i]);
	}
	return std::string();
}

void Args::print_version()
{
	std::cout << "Joao v" << VERSION_STRING <<"\tCopyright (C) 2021 Altoids1 (mom-puter@hotmail.com)\n";
}
void Args::print_help()
{
	std::cout << "Acceptable arguments: [option] [script [args]]\n";
	std::cout << "Available options are:\n";
	std::cout << "  -v\tshow version information\n";
	std::cout << "  -h\tshow help dialog\n";
	std::cout << "  -i\tenter interactive mode\n";
	std::cout << "  -m\tprints the output of /main() into stdout after program completion\n";
	std::cout << "  -e\tprints the execution time. On by default when compiled with _DEBUG.\n";
}

//Tries to run the given chunk of strings as if they were all statements in the /main(){} of a program with nothing else in it.
//Returns true if successful and false if an error occurred.
bool Args::run_code_block(std::vector<std::string>& statements)
{
	std::stringstream dummy;

	dummy << "/main(){\n";

	for (const std::string& s : statements)
	{
		dummy << s << std::endl;
	}

	dummy << "return 0;\n}\n";

	Scanner scn(true);
	scn.scan(dummy);

	if (scn.is_malformed)
		return false;

	Parser prs(scn);
	Program prog = prs.parse();

	if (prog.is_malformed)
		return false;

	Interpreter interp(prog,true);
	Value jargs = interp.makeBaseTable();
	interp.execute(prog, jargs);

	return !(interp.error);
}

static Program interactive_default_program() {
	std::stringstream dummy_code;
	dummy_code << "/main(){return 0;}/quit(){ throw /error/New(1,\"Calling quit() in this way is not yet implemented!\");}";
	Scanner scn;
	scn.scan(dummy_code);
	if(scn.is_malformed)
		throw std::runtime_error("interactive's default program failed to scan! This is a bug :(");
	Parser prs(scn);
	Program ret = prs.parse();
	if(ret.is_malformed)
		throw std::runtime_error("interactive's default program failed to scan! This is a bug :(");
	return ret;
} 

static FailureOr try_run_expression(Program& prog, std::string&& expr_str) {
	std::stringstream expr_ss(expr_str);
	Scanner scn(true);
	scn.scan(expr_ss);
	if(scn.is_malformed)
		return FailureOr(ErrorCode::Unknown,"Scanning failed.");
	Parser prs(scn);
	ASTNode* ptr = prs.parse_expression();
	if(ptr == nullptr)
		return FailureOr(ErrorCode::Unknown,"Parsing failed.");
	Interpreter interp(prog,true);
	Value ret = interp.evaluate_expression(ptr);
	if(interp.error)
		return FailureOr(std::move(interp.error));
	return ret;
}

void Args::interactive_mode()
{
	Program prog = interactive_default_program();
	while (true)
	{
		std::cout << "> ";
		std::string input = "";
		std::getline(std::cin, input);

		if(input == "")
			continue;
		if(input == "quit()") // FIXME: support quit() more generically
			return;
		FailureOr result = try_run_expression(prog, std::move(input));
		if(!result.didError)
			std::cout << result.must_get().to_string() << std::endl;
	}
}
