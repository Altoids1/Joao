#include "Object.h"
#include "Interpreter.h"
#include "Parser.h"
#include "Table.h"

Value* Object::has_property(Interpreter& interp, const ImmutableString& name)
{
	if (properties.count(name))
		return &(properties.at(name));
	if (base_properties->count(name))
		return &(base_properties->at(name));

	return nullptr;
}
Value Object::get_property(Interpreter& interp, const ImmutableString& name)
{
	if (properties.count(name))
		return properties.at(name);
	if (base_properties->count(name))
		return base_properties->at(name);
	
	interp.RuntimeError(nullptr, ErrorCode::BadMemberAccess, "Unable to access property of object!");
	return Value();
}
Value Object::get_property_raw(const ImmutableString& name)
{
	if (properties.count(name))
		return properties.at(name);
	if (base_properties->count(name))
		return base_properties->at(name);
	return Value();
}
void Object::set_property(Interpreter& interp, const ImmutableString& name, Value rhs)
{
	if (base_properties->count(name) == 0)
	{
		interp.RuntimeError(nullptr, ErrorCode::BadMemberAccess, "Unable to access property of object!");
		return;
	}
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
void Object::set_property_raw(const ImmutableString& name, Value rhs)
{
	if (base_properties->count(name) == 0)
		return;

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

Value Object::call_method(Interpreter& interp, const ImmutableString& name, std::vector<Value> &args)
{
	Function* fuh;

	if (base_funcs->count(name)) // Note how it checks the typical methods before trying to invoke any metamethods.
	{
		fuh = base_funcs->at(name);
	}
	else if (mt && mt->metamethods.contains(name))
	{
		fuh = (mt->metamethods.at(name));
	}
	else
	{
		interp.RuntimeError(nullptr, ErrorCode::BadMemberAccess, "Unable to access method of object: " + name.to_string());
		return Value();
	}
	
	fuh->give_args(interp, args, this);
	fuh->set_obj(this);
	return fuh->resolve(interp);
}

//If it fails, it simply returns a nullptr w/o throwing a Runtime. Part of scope resolution of function calls.
Function* Object::has_method(Interpreter& interp, const ImmutableString& name)
{
	if (base_funcs->count(name))
	{
		return base_funcs->at(name);
	}
	else if (mt && mt->metamethods.contains(name))
	{
		return mt->metamethods.at(name);
	}
	return nullptr;
}

/* Object Type */

Object* ObjectType::makeObject(Interpreter& interp, std::vector<Value>&& args)
{
	Object* o;
	if (is_table_type)
		o = new Table(object_type, &typeproperties, &typefuncs);
	else
		o = new Object(object_type, &typeproperties, &typefuncs, mt);

	if (typefuncs.count("#constructor"))
	{
		Function* fuh = typefuncs["#constructor"]; //fuh.
		fuh->give_args(interp, args, o);
		fuh->resolve(interp);
	}
	else if(mt)
	{
		NativeMethod** fuh = mt->metamethods.lazy_at("#constructor");
		if(fuh)
		{
			(*fuh)->give_args(interp, args, o);
			(*fuh)->resolve(interp);
		}
		o->mt = mt;
	}

	return o;
}

Value ObjectType::get_typeproperty(Interpreter& interp, const ImmutableString& str, ASTNode* getter)
{
	if (!typeproperties.count(str))
	{
		interp.RuntimeError(getter, ErrorCode::BadMemberAccess, "Failed to access property " + str.to_string() + " of grandparent " + object_type.to_string() + "!");
		return Value();
	}
	return typeproperties.at(str);
}

Function* ObjectType::has_typemethod(Interpreter& interp, const ImmutableString& str, ASTNode* getter)
{
	if (!typefuncs.count(str))
	{
		if (mt)
		{
			NativeMethod** native = mt->metamethods.lazy_at(str);
			if (native)
				return *native;
		}
		return nullptr;
	}
	return typefuncs.at(str);
}

void ObjectType::set_typeproperty(Parser& parse, const ImmutableString& name, const Value& v)
{
	if (typeproperties.count(name))
	{
		parse.ParserError(nullptr, "Duplicate property of ObjectType detected!");
	}

	typeproperties[name] = v;
}

void ObjectType::set_typeproperty_raw(const ImmutableString& name, Value v)
{
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

void ObjectType::set_typemethod_raw(const ImmutableString& name, Function* f)
{
	typefuncs[name] = f;
}