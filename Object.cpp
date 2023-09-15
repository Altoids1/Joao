#include "Object.h"
#include "Interpreter.h"
#include "Parser.h"
#include "Table.h"
#include "Value.h"

Value* Object::has_property(Interpreter& interp, const ImmutableString& name)
{
	Value* ret;
	if (ret = properties.lazy_at(name); ret != nullptr)
		return ret;
	return base_properties->lazy_at(name);
}
Value Object::get_property(Interpreter& interp, const ImmutableString& name)
{
	Value* ptr;
	if (ptr = properties.lazy_at(name); ptr != nullptr)
		return *ptr;
	if (ptr = base_properties->lazy_at(name); ptr != nullptr)
		return *ptr;
	
	interp.RuntimeError(nullptr, ErrorCode::BadMemberAccess, "Unable to access property of object!");
	return Value();
}
Value Object::get_property_raw(const ImmutableString& name)
{
	Value* ptr;
	if (ptr = properties.lazy_at(name); ptr != nullptr)
		return *ptr;
	if (ptr = base_properties->lazy_at(name); ptr != nullptr)
		return *ptr;
	return Value();
}
void Object::set_property(Interpreter& interp, const ImmutableString& name, Value rhs)
{
	if (base_properties->count(name) == 0)
	{
		interp.RuntimeError(nullptr, ErrorCode::BadMemberAccess, "Unable to access property of object!");
		return;
	}
	properties[name] = rhs;
}
void Object::set_property_raw(const ImmutableString& name, Value rhs)
{
	if (base_properties->count(name) == 0)
		return;

	properties[name] = rhs;
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
	if (Function** ptr = base_funcs->lazy_at(name); ptr != nullptr)
	{
		return *ptr;
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

std::string Object::to_json()  { 
	std::string jsonOutput = "{";
	jsonOutput.reserve(512); // I hereby promote you to StringBuilder
	jsonOutput += math::concat("\"__TYPE__\":\"",object_type.data,"\"");
	if(this->is_table()) {
		const Table* self = static_cast<const Table*>(this);
		jsonOutput += ",\"__TABLE__\" : [[";
		//first the array
		for(const Value& v : self->t_array) {
			jsonOutput += v.to_json() + ",";
		}
		jsonOutput.pop_back(); // No trailing commas! :^)
		jsonOutput += "],{";
		//second, the hashtable portion
		for(auto it = self->t_hash.begin(); it != self->t_hash.end(); ++it) {
			//Mindlessly using a Value as a JSON member key may seem suspicious
			//(since JSON member keys can *only* be strings)
			//however, João tables may only have integers and strings as keys. This should be okay!
			jsonOutput += math::concat("\"",it.key().to_string(),"\":",it.value().to_json(),",");
		}
		jsonOutput.pop_back(); // Delete the trailing comma :^)
		jsonOutput += "}]";
	}
	for(auto it = base_properties->begin(); it != base_properties->end(); ++it) {
		//Comma is placed at the front to avoid having a trailing comma.
		//(we'll always need this first comma since the TYPE element always comes before this)
		jsonOutput += math::concat(",\"",it.key().data,"\":",get_property_raw(it.key()).to_json());
	}

	jsonOutput += "}";
	return jsonOutput;
}
