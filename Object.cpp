#include "Object.h"
#include "Interpreter.h"
#include "Parser.h"
#include "Table.h"

Value* Object::has_property(Interpreter& interp, std::string name)
{
	if (properties.count(name))
		return &(properties.at(name));
	if (base_properties->count(name))
		return &(base_properties->at(name));

	return nullptr;
}
Value Object::get_property(Interpreter& interp, std::string name)
{
	if (properties.count(name))
		return properties.at(name);
	if (base_properties->count(name))
		return base_properties->at(name);
	
	interp.RuntimeError(nullptr, "Unable to access property of object!");
	return Value();
}
void Object::set_property(Interpreter& interp, std::string name, Value rhs)
{
	if (base_properties->count(name) == 0)
		interp.RuntimeError(nullptr, "Unable to access property of object!");

	if (properties.count(name))
	{
		properties.erase(name);
		properties[name] = rhs;
	}
	else
	{
		properties[name] = rhs;
	}
}

Value Object::call_method(Interpreter& interp, std::string name, std::vector<Value> &args)
{
	if (!base_funcs->count(name))
		interp.RuntimeError(nullptr, "Unable to access method of object: " + name);

	Function* fuh = base_funcs->at(name);
	fuh->give_args(interp, args, this);
	fuh->set_obj(this);
	return fuh->resolve(interp);
}

//If it fails, it simply returns a nullptr w/o throwing a Runtime. Part of scope resolution of function calls.
Function* Object::get_method(Interpreter& interp, std::string name)
{
	if (base_funcs->count(name))
	{
		return base_funcs->at(name);
	}
	return nullptr;
}

/* Object Type */

Object* ObjectType::makeObject(Interpreter& interp, std::vector<Value>& args)
{
	Object* o;
	if (is_table_type)
		o = new Table(object_type, &typeproperties, &typefuncs);
	else
		o = new Object(object_type, &typeproperties, &typefuncs); // FIXME: Make these instantiated objects capable of being garbage-collected; this is a memory leak right now!
	if (typefuncs.count("#constructor"))
	{
		Function* fuh = typefuncs["#constructor"]; //fuh.
		if (args.size())
			fuh->give_args(interp, args, o);
		fuh->resolve(interp);
	}
	return o;
}

Value ObjectType::get_typeproperty(Interpreter& interp, std::string str, ASTNode* getter)
{
	if (!typeproperties.count(str))
	{
		interp.RuntimeError(getter, "Failed to access property " + str + " of grandparent " + object_type + "!");
		return Value();
	}
	return typeproperties.at(str);
}


void ObjectType::set_typeproperty(Parser& parse, std::string name, Value v)
{
	if (typeproperties.count(name))
	{
		parse.ParserError(nullptr, "Duplicate property of ObjectType detected!");
	}

	typeproperties[name] = v;
}

void ObjectType::set_typemethod(Parser& parse, std::string name, Function* f)
{
	if (typefuncs.count(name))
	{
		parse.ParserError(nullptr, "Duplicate method of ObjectType detected!");
	}

	typefuncs[name] = f;
}

void ObjectType::set_typemethod_raw(std::string name, Function* f)
{
	typefuncs[name] = f;
}