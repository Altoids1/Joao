#include "Forward.h"
#include "AST.h"
#include "Interpreter.h"
#include "Object.h"
#include "Table.h"

Interpreter::Interpreter()
	:is_interactive(false)
{

}

//Fair warning: In this context Interpreter doesn't ensure that its Program will not be garbage collected in a higher scope.
Interpreter::Interpreter(Program& p, bool interact = false)
	:is_interactive(interact)
{
	prog = &p;
	globalscope = p.globals;
}

Value Interpreter::execute(Program& program, Value& jarg)
{
	prog = &program;

	Function* main = prog->get_func("main");

	if (!main)
	{
		RuntimeError(main, "/main() is not defined!");
		exit(1); // HAS to be a forced-exit since this crashes the .exe otherwise!
	}

	std::vector<Value> jargs = {jarg};
	main->give_args(*this,jargs,nullptr);
	return main->resolve(*this);
}

void Interpreter::RuntimeError(ASTNode* a, std::string what)
{
	std::cout << "- - - - - - - - - - - - - - - -\n";
	std::cout << "FATAL RUNTIME ERROR: " << what << "\n";
	
	//Stack dump
	std::string whatfunk;
	if(objectscope.top()) // If we runtimed within a method
	{
		whatfunk = "method of object of type " + objectscope.top()->object_type;
	}
	else
	{
		whatfunk = "global function";
	}
	
	std::cout << "Line number: ";
	if(a && a->my_line)
	{
		std::cout << std::to_string(a->my_line);
	}
	else
	{
		std::cout << "Unknown!"; // Eventually things should be configured to never do this
	}
	
	std::cout << "\nRuntime in " << blockscope.top().get_back_name().to_string() << ", a " << whatfunk << std::endl;
	if(!is_interactive)
		exit(1);
}

void Interpreter::RuntimeError(ASTNode* node, ErrorCode err,const std::string& what)
{
	error = Value(prog->definedObjTypes["/error"]->makeObject(*this, {Value(static_cast<int>(err)),Value(what) }));
	//assert(error.t_value.as_object_ptr->get_property_raw("what").t_vType == Value::vType::String);
	return;
}

void Interpreter::RuntimeError(ASTNode* node, Value& err_val)
{
	error = err_val;
	return;
}
void Interpreter::UncaughtRuntime(const Value& err)
{
	std::cout << *(err.t_value.as_object_ptr->get_property(*this, "what").t_value.as_string_ptr);
#ifdef JOAO_SAFE
	throw err;
#else
	int code = err.t_value.as_object_ptr->get_property(*this, "code").t_value.as_int;
	if (code)
		exit(code);
	else
		exit(-(1 << 16));
#endif
}

void Interpreter::init_var(const ImmutableString& varname, const Value& val, ASTNode* setter)
{
#ifdef JOAO_SAFE
	++value_init_count;
	if (value_init_count > MAX_VARIABLES)
	{
		throw error::max_variables(std::string("Program reached the limit of ") + std::to_string(MAX_VARIABLES) + std::string("instantiated variables!"));
	}
#endif
	blockscope.top().set(varname, val);
}

void Interpreter::override_var(std::string varname, Value val, ASTNode* setter)
{
	//First try blockscope
	Scope<Value>& varscope = blockscope.top();
	Value* vptr = varscope.get(varname);
	if (vptr)
	{
		varscope.Override(varname, val);
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
		globalscope.table[varname] = val;
		return;
	}

	//Give up. How the hell did we even fail to set it in globalscope?
	RuntimeError(setter, "Unable to override value of variable named " + varname + "!");
}

Value& Interpreter::get_var(const ImmutableString& varname, ASTNode *getter)
{
	//First try blockscope
	Scope<Value>& varscope = blockscope.top();
	Value* vptr = varscope.get(varname);
	if (vptr)
		return *vptr;

	//Then try objectscope
	Object* objscope = objectscope.top();
	if(objscope)
	{
		vptr = objscope->has_property(*this, varname.to_string());
		if (vptr)
			return *vptr;
	}

	//Then try globalscope
	Value* globptr = globalscope.table.lazy_at(varname);
	if (globptr)
		return *globptr;

	//Give up :(
	RuntimeError(getter, ErrorCode::BadAccess, std::string("Unable to access variable named ") + varname.to_string() + std::string("!"));
	return Value::dev_null;
}

Function* Interpreter::get_func(const std::string& funkname, ASTNode *caller, bool loud)
{
	//Try to find an objectscope function with this name

	Object* obj = objectscope.top();
	if (obj)
	{
		Function* method = obj->has_method(*this, funkname);
		if (method)
			return method;
	}

	//Try to find a global function with this name
	if (prog->definedFunctions.count(funkname))
	{
		return prog->get_func(funkname);
	}

	if(loud)
		RuntimeError(caller, ErrorCode::BadAccess, "Failed to find function named " + funkname + "!");
	return nullptr;
}

Value Interpreter::get_property(std::string str, ASTNode* getter)
{
	Object* obj = objectscope.top();
	if (obj)
	{
		return obj->get_property(*this,str);
	}
	RuntimeError(getter, ErrorCode::BadMemberAccess, "Failed to get property of Parent objectscope!");
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
	RuntimeError(getter, ErrorCode::BadMemberAccess, "Failed to set property of Parent objectscope!");
	return;
}

Value Interpreter::grand_property(unsigned int depth, std::string str, ASTNode* getter)
{
	Object* obj = objectscope.top();
	if (!obj)
	{
		RuntimeError(getter, ErrorCode::BadMemberAccess, "Cannot get grandparent property in classless objectscope!");
		return Value();
	}

	std::string dir = obj->object_type;

	for (unsigned int i = 0; i < depth; ++i)
	{
		dir = Directory::DotDot(dir);
	}
	if (dir == "/")
	{
		RuntimeError(getter, ErrorCode::BadMemberAccess, "Attempted to get grandparent of root directory!");
		return Value();
	}

	if (!prog->definedObjTypes.count(dir))
	{
		RuntimeError(getter, ErrorCode::BadMemberAccess, "Failed to find objecttype of type " + dir + " during GrandparentAccess!"); // This'll be a weird error to get once inheritence is functional
		return Value();
	}

	ObjectType* objt = prog->definedObjTypes.at(dir);
	return objt->get_typeproperty(*this, str, getter);
}

Value& Interpreter::grand_handle(unsigned int depth, std::string str, ASTNode* getter)
{
	Object* obj = objectscope.top();
	if (!obj)
	{
		RuntimeError(getter, ErrorCode::BadMemberAccess, "Cannot get grandparent handle in classless objectscope!");
		return Value::dev_null;
	}

	std::string dir = obj->object_type;

	for (unsigned int i = 0; i < depth; ++i)
	{
		std::string dir = Directory::DotDot(dir);
	}
	if (dir == "/")
	{
		RuntimeError(getter, ErrorCode::BadMemberAccess, "Attempted to get grandparent handle of root directory!");
		return Value::dev_null;
	}

	if (!prog->definedObjTypes.count(dir))
	{
		RuntimeError(getter, ErrorCode::BadMemberAccess, "Failed to find objecttype of type " + dir + " during GrandparentAccess!"); // This'll be a weird error to get once inheritence is functional
		return Value::dev_null;
	}

	ObjectType* objt = prog->definedObjTypes.at(dir);

	//If it's a property
	Value* vuh = objt->has_typeproperty(*this, str, getter);
	if (vuh)
	{
		return *vuh;
	}

	//If it's a method
	Function* fnuh = objt->has_typemethod(*this, str, getter);
	if (fnuh)
	{
		return fnuh->to_value();
	}

	RuntimeError(getter, ErrorCode::BadMemberAccess, "Failed to get property of Parent objectscope!");
	return Value::dev_null;
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

	return Value(prog->definedObjTypes[str]->makeObject(*this, std::move(eval_args)));
}
Value Interpreter::makeObject(std::string str, std::vector<Value>&& eval_args, ASTNode* maker)
{

	if (!(prog->definedObjTypes.count(str)))
		RuntimeError(maker, "Constructor attempts to instantiate unknown type! (" + str + ")"); // FIXME: This should really be a Parsetime error
	return Value(prog->definedObjTypes[str]->makeObject(*this, std::move(eval_args)));
}

Value Interpreter::makeBaseTable()
{
	return Value(prog->definedObjTypes["/table"]->makeObject(*this, {}));
}

Value Interpreter::makeBaseTable( std::vector<Value> elements, Hashtable<std::string,Value> entries, ASTNode* maker = nullptr)
{
	Object* objdesk = prog->definedObjTypes["/table"]->makeObject(*this,{});
	
	Table* desk = static_cast<Table*>(objdesk);
	for(size_t i = 0; i < elements.size(); ++i)
	{
		desk->at_set(*this,Value(i), elements[i]);
	}
	
	for(auto it = entries.begin(); it != entries.end(); ++it)
	{
		desk->at_set(*this,Value(it.key()), it.value());
	}
	
	return Value(objdesk);
}