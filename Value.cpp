#include "Value.h"
#include "Object.h"

std::unordered_map<Object*, std::shared_ptr<Object>*> Value::known_objptrs;

Value::Value(std::string s)
{
	strptr = std::make_shared<std::string>(s);
	t_vType = vType::String;
}

/*
So we have this treat this a lil bit special since this pointer might actually point to an object that we share with someone else,
and the current (kinda crap) memory management system in-place can result in multiple shared_ptrs being created for the same Object*, causing crashy behaviour from overdeleting it.
*/
Value::Value(Object* o)
{
	if (known_objptrs.count(o))
	{
		objptr = *known_objptrs.at(o);
	}
	else
	{
		objptr = std::shared_ptr<Object>(o);

	}
	t_vType = vType::Object;
}
Value::~Value()
{
	if (objptr.use_count() == 1)
	{
		std::cout << "I'm about to delete this object:\t" << objptr->dump();
	}
	if (strptr.use_count() == 1)
	{
		std::cout << "I'm about to delete this string:\t" << *strptr;
	}
	std::cout << "*Yoda Scream*\n";
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