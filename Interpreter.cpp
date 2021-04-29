#include "Forward.h"
#include "AST.h"
#include "Interpreter.h"
#include "Object.h"

Interpreter::Interpreter()
{

}

Value Interpreter::execute(Program& program)
{
	prog = &program;

	Function* main = prog->get_func("/main");

	if (!main)
	{
		RuntimeError(main, "/main() is not defined!");
		exit(1); // HAS to be a forced-exit since this crashes the .exe otherwise!
	}

	return main->resolve(*this);
}

void Interpreter::set_var(std::string varname, Value val, ASTNode* setter)
{
	//std::cout << "Setting variable " + varname + " to value " + std::to_string(val.t_value.as_int) + "\n";
	varscope.set(varname, val);
}

void Interpreter::override_var(std::string varname, Value val, ASTNode* setter)
{
	//std::cout << "Overriding variable " + varname + " to value " + std::to_string(val.t_value.as_int) + "\n";
	if (!varscope.Override(varname, val))
	{
		RuntimeError(setter, "Unable to override blockscope variable with new value!");
	}
	return;
}

Value Interpreter::get_var(std::string varname, ASTNode *getter)
{
	Value* vptr = varscope.get(varname);
	if (vptr == nullptr)
	{
		RuntimeError(*getter,"Unable to access variable named " + varname + "!");
		return Value();
	}
	return *vptr;
}

Function* Interpreter::get_func(std::string funkname, ASTNode *caller)
{

	std::string objscope = objscopestr;
	while (true)
	{
		Function* f = prog->get_func(objscopestr + "/" + funkname); // Try the localest obj scope
		if (f != nullptr) return f; // if we find it, cool
		//else, go up a scope and try again!

		if (objscope == "") break; // If we've already hit root, then just abort.
		objscope = Directory::DotDot(objscope);
	}
	RuntimeError(*caller,"Failed to find function named " + funkname + "!");
	return nullptr;
}

Value Interpreter::makeObject(std::string str, std::vector<ASTNode*>& args, ASTNode* maker)
{

	if (!(prog->definedObjTypes.count(str)))
		RuntimeError(maker, "Constructor attempts to instantiate unknown type! (" + str + ")"); // FIXME: This should really be a Parsetime error
	
	std::vector<Value> eval_args;
	for (size_t i = 0; i < args.size(); ++i)
	{
		eval_args.push_back(args[i]->resolve(*this));
	}

	return Value(prog->definedObjTypes[str]->makeObject(*this, eval_args));
}