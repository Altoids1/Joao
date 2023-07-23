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
	delete ptr; // FIXME: be RAII about this, come on.
	if(interp.error)
		return FailureOr(std::move(interp.error));
	return ret;
}

#ifdef __EMSCRIPTEN__ 
#include <emscripten.h>
//In Emscripten's current setup, innocently doing std::getline() with std::cin actually causes us
//to hammer the external JS function that is feeding us stuff from stdin,
//as well as the fake /dev/tty.
//
//This means that we'll have to get kinda funky with it if we want to get user input w/o the front-end being a laggy, spinny, crashy mess.
//
//Make sure to use the -sASYNCIFY arg to em++ to make this work.
//meson.build *SHOULD* have it as a default linker argument. Try not to override it or the whole binary may just, not work at runtime.
static void detail_get_line(std::string& ret) 
{
	using std::cin;
	char tryget;
	while(true) {
		tryget = cin.get();
		if(std::char_traits<char>::not_eof(tryget))
			break;
		emscripten_sleep(100); // To avoid hammering it for characters.
	}
	ret.push_back(tryget);
	while(true) {
		tryget = cin.get();
		if(tryget == std::char_traits<char>::eof() || !cin.good())
			return;
		if(tryget == '\n' || tryget == '\r')
			return;
		ret.push_back(tryget);
	}
	return;
}
#endif

void Args::interactive_mode()
{
	Program prog = interactive_default_program();
	while (true)
	{
		std::cout << "> ";
		std::string input;
#ifdef __EMSCRIPTEN__ // Explanation at the top of detail_get_line()'s def/impl.
		detail_get_line(input);
#else
		std::getline(std::cin, input);
#endif

		if(input.empty())
			continue;
		if(input == "quit()") // FIXME: support quit() more generically
			return;
		FailureOr result = try_run_expression(prog, std::move(input));
		if(!result.didError)
			std::cout << result.must_get().to_string() << std::endl;
	}
}