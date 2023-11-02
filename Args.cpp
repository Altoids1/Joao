#include "Args.h"

#include "Scanner.h"
#include "Parser.h"
#include "Interpreter.h"
#include "FailureOr.h"
#include "Terminal.h"
#include "Object.h"
#include "Error.h"

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
		if (_strcmpi(argv[i], "-d") == 0)
		{
			v.push_back(Flags::InitializeDaemon);
			continue;
		}
		if (_strcmpi(argv[i], "-f") == 0)
		{
			v.push_back(Flags::DisableFormatting);
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
	std::cout << "Joao v" << VERSION_STRING <<"\tCopyright (C) 2021-2023 Altoids1 (mom-puter@hotmail.com)\n";
}
void Args::print_help()
{
	std::cout << "Acceptable arguments: [option] [script [args]]\n";
	std::cout << "Available options are:\n";
	std::cout << "  -v\tshow version information\n";
	std::cout << "  -h\tshow help dialog\n";
	std::cout << "  -i\tenter interactive mode\n";
	std::cout << "  -m\tprints the output of /main() into stdout after program completion, in JSON format.\n";
	std::cout << "  -e\tprints the execution time. On by default when compiled with _DEBUG.\n";
#if defined(__linux__) && defined(JOAO_SAFE)
	std::cout << "  -d\tInitializes Joao as a daemon, instead of executing a script.\n";
#endif
	std::cout << "  -f\tDisables colour (and other formatting) when emitting errors or messages.\n";
}
}

static Program interactive_default_program() {
	std::stringstream dummy_code;
	dummy_code << "/main(){return 0;}/quit(){ throw /error/New(0,\"REPL exited via quit() call.\");}";
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

static std::optional<Value> try_run_expression(Interpreter& interp, Program& prog, std::string&& expr_str) {
	std::stringstream expr_ss(expr_str);
	Scanner scn(true);
	scn.scan(expr_ss);
	if(scn.is_malformed)
		return {};
	Parser prs(scn);
	ASTNode* ptr;
	try {
		ptr = prs.parse_repl_expression(prog);
	} catch (error::parser& err) {
		return {};
	}
	if(ptr == nullptr)
		return {};
	
	Value ret = interp.evaluate_expression(ptr);
	delete ptr; // FIXME: be RAII about this, come on.
	if(interp.error) {
		//In this case, we have to display this error value manually.
		if(interp.error.t_vType == Value::vType::Object) {
			Object* obj = interp.error.t_value.as_object_ptr;
			if(Value* whatPtr = obj->has_property(interp,"what"); whatPtr != nullptr) {
				Terminal::SetColor(std::cout, Terminal::Color::Red);
				std::cout << whatPtr->to_string() << '\n';
				Terminal::ClearFormatting(std::cout);
				return {};
			}
		}
		Terminal::SetColor(std::cout, Terminal::Color::Red);
		std::cout << interp.error.to_string();
		Terminal::ClearFormatting(std::cout);
		return {};
	}
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
	Interpreter interp(prog,true);
	interp.push_stack("#repl");
	while (true)
	{
		Terminal::SetColor(std::cout,Terminal::Color::Red);
		std::cout << "> ";
		Terminal::SetColor(std::cout,Terminal::Color::RESET);
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
		if(input.back() != ';')
			input.push_back(';');
		auto result = try_run_expression(interp, prog, std::move(input));
		if(result)
			std::cout << result.value().to_string() << std::endl;
	}
}