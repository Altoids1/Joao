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

	Function* main = prog->get_func("main");

	if (!main)
	{
		RuntimeError(main, "/main() is not defined!");
		exit(1); // HAS to be a forced-exit since this crashes the .exe otherwise!
	}

	return main->resolve(*this);
}

void Interpreter::init_var(std::string varname, Value val, ASTNode* setter)
{
	Scope<Value>* varscope = blockscope.top();
	
	if (varscope->get_front(varname))
		RuntimeError(setter, "Cannot initialize the same variable at the same blockscope multiple times!");

	varscope->set(varname, val);
}

void Interpreter::override_var(std::string varname, Value val, ASTNode* setter)
{
	//First try blockscope
	Scope<Value>* varscope = blockscope.top();
	Value* vptr = varscope->get(varname);
	if (vptr)
	{
		varscope->Override(varname, val);
		return;
	}

	//Then try objectscope
	Object* objscope = objectscope.top();
	if (objscope)
	{
		vptr = objscope->has_property(*this, varname);
		if (vptr)
		{
			objscope->set_property(*this, varname, val);
			return;
		}
	}

	//Then try globalscope
	if (globalscope.table.count(varname))
	{
		Value* vuh = new Value(val);
		globalscope.table[varname] = vuh;
		return;
	}

	//Give up. How the hell did we even fail to set it in globalscope?
	RuntimeError(*setter, "Unable to override value of variable named " + varname + "!");
}

Value Interpreter::get_var(std::string varname, ASTNode *getter)
{
	//First try blockscope
	Scope<Value>* varscope = blockscope.top();
	Value* vptr = varscope->get(varname);
	if (vptr)
		return Value(*vptr);

	//Then try objectscope
	Object* objscope = objectscope.top();
	if(objscope)
	{
		vptr = objscope->has_property(*this, varname);
		if (vptr)
			return Value(*vptr);
	}

	//Then try globalscope
	if (globalscope.table.count(varname))
	{
		return *globalscope.table.at(varname);
	}

	//Give up :(
	RuntimeError(*getter,"Unable to access variable named " + varname + "!");
	return Value();
}

Function* Interpreter::get_func(std::string funkname, ASTNode *caller, bool loud)
{
	//Try to find an objectscope function with this name
	Object* obj = objectscope.top();
	if (obj)
	{
		Function* method = obj->get_method(*this, funkname);
		if (method)
			return method;
	}

	//Try to find a global function with this name
	if (prog->definedFunctions.count(funkname))
	{
		return prog->get_func(funkname);
	}

	if(loud)
		RuntimeError(*caller,"Failed to find function named " + funkname + "!");
	return nullptr;
}

Value Interpreter::get_property(std::string str, ASTNode* getter)
{
	Object* obj = objectscope.top();
	if (obj)
	{
		return obj->get_property(*this,str);
	}
	RuntimeError(getter, "Failed to get property of Parent objectscope!");
	return Value();
}

void Interpreter::set_property(std::string str, Value val, ASTNode* getter)
{

	Object* obj = objectscope.top();
	if (obj)
	{
		obj->set_property(*this, str, val);
		return;
	}
	RuntimeError(getter, "Failed to set property of Parent objectscope!");
	return;
}

Value Interpreter::grand_property(unsigned int depth, std::string str, ASTNode* getter)
{
	Object* obj = objectscope.top();
	if (!obj)
	{
		RuntimeError(getter, "Cannot get grandparent property in classless objectscope!");
		return Value();
	}

	std::string dir = obj->object_type;

	for (unsigned int i = 0; i < depth; ++i)
	{
		std::string dir = Directory::DotDot(dir);
	}
	if (dir == "/")
	{
		RuntimeError(getter, "Attempted to get grandparent of root directory!");
		return Value();
	}

	if (!prog->definedObjTypes.count(dir))
	{
		RuntimeError(getter, "Failed to find objecttype of type " + dir + " during GrandparentAccess!"); // This'll be a weird error to get once inheritence is functional
		return Value();
	}

	ObjectType* objt = prog->definedObjTypes.at(dir);
	return objt->get_typeproperty(*this, str, getter);
	
	
	if (obj)
	{
		return obj->get_property(*this, str);
	}
	RuntimeError(getter, "Failed to get property of Parent objectscope!");
	return Value();
}

Handle Interpreter::grand_handle(unsigned int depth, std::string str, ASTNode* getter)
{
	Object* obj = objectscope.top();
	if (!obj)
	{
		RuntimeError(getter, "Cannot get grandparent handle in classless objectscope!");
		return Handle();
	}

	std::string dir = obj->object_type;

	for (unsigned int i = 0; i < depth; ++i)
	{
		std::string dir = Directory::DotDot(dir);
	}
	if (dir == "/")
	{
		RuntimeError(getter, "Attempted to get grandparent handle of root directory!");
		return Handle();
	}

	if (!prog->definedObjTypes.count(dir))
	{
		RuntimeError(getter, "Failed to find objecttype of type " + dir + " during handle() of GrandparentAccess!"); // This'll be a weird error to get once inheritence is functional
		return Handle();
	}

	ObjectType* objt = prog->definedObjTypes.at(dir);

	//If it's a property
	Value* vuh = objt->has_typeproperty(*this, str, getter);
	if (vuh)
	{
		Handle huh;
		huh.type = Handle::HType::ObjType;
		huh.name = str;
		huh.data.objtype = objt;
		return huh;
	}

	//If it's a method
	Function* fnuh = objt->has_typemethod(*this, str, getter);
	if (fnuh)
	{
		Handle huh;
		huh.type = Handle::HType::ObjType;
		huh.name = str;
		huh.data.objtype = objt;
		huh.is_function = true;
		return huh;
	}

	RuntimeError(getter, "Failed to get property of Parent objectscope!");
	return Handle();
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