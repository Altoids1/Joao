#include "Forward.h"
#include "AST.h"
#include "Interpreter.h"
#include "Directory.h"

Interpreter::Interpreter()
{

}

Value Interpreter::execute(Program& program)
{
	prog = program;
	return prog.get_func("main")->resolve(*this);
}

Function* Interpreter::get_func(std::string str)
{
	//First resolve the scope
	std::string newstr = str;


	
	//*Then* get & return the function
	Function* f = prog.get_func(newstr);
	return f;
}