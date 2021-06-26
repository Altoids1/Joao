#include "Value.h"
#include "Object.h"

Value::Value(const std::string& s) // Memory-leaky, not to be used by the interpreter
{
	t_value.as_str_ptr = new std::string(s);
	t_vType = vType::String;
}

Value::Value(std::string* s)
{
	t_value.as_str_ptr = s;
	t_vType = vType::String;
}

/*
So we have this treat this a lil bit special since this pointer might actually point to an object that we share with someone else,
and the current (kinda crap) memory management system in-place can result in multiple shared_ptrs being created for the same Object*, causing crashy behaviour from overdeleting it.
*/
Value::Value(Object* o)
{
	t_value.as_obj_ptr = o;
	t_vType = vType::Object;
}

std::string Value::to_string()
{
	switch (t_vType)
	{
	case(vType::Null):
		return "NULL";
	case(vType::Bool):
		return std::to_string(t_value.as_bool);
	case(vType::Integer):
		return std::to_string(t_value.as_int);
	case(vType::Double):
		return std::to_string(t_value.as_double);
	case(vType::String):
		//std::cout << *t_value.as_str_ptr;
		return *strget();
	case(vType::Object):
		return objget()->dump();
	default:
		return "???";
	}
}

std::string Value::typestring()
{
	switch (t_vType)
	{
	case(vType::Null):
		return "NULL";
	case(vType::Bool):
		return "Boolean";
	case(vType::Integer):
		return "Integer";
	case(vType::Double):
		return "Double";
	case(vType::String):
		return "String";
	case(vType::Object):
		return "Object";
	default:
		return "UNKNOWN!!";
	}
}