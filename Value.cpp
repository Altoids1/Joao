#include "Forward.h"
#include "Value.h"
#include "Object.h"

Hashtable<void*, uint32_t> Value::cached_ptrs;
std::unordered_set<std::string> Value::cached_strings;
Value Value::dev_null = Value();

Value::Value(Object* o)
	:UnmanagedValue(o)
{
	if(cached_ptrs.contains(o)) // Novel Object, I guess?
	{
		cached_ptrs.insert(o,1u);
	}
	else
	{
		cached_ptrs[o]++;
	}
}

void Value::deref_as_obj()
{
#ifdef _DEBUG
	int cnt = cached_ptrs.count(t_value.as_object_ptr);
	if (!cnt)
	{
		std::cout << "Found object " << to_string() << " unregistered to Value ptr cache!\n";
		exit(1);
	}
#endif
	if (--cached_ptrs[t_value.as_object_ptr] == 0)
	{
		cached_ptrs.erase(t_value.as_object_ptr);
#ifdef LOUD_GC
		std::cout << "Deleting " << to_string() << std::endl;
#endif
		delete t_value.as_object_ptr;
	}
}

Value::Value(const Value& v)
{
	//std::cout << "Copying " << v.to_string() << " to a new Value!\n";
	t_vType = v.t_vType;
	switch (t_vType)
	{
	case(vType::Object):
		t_value.as_object_ptr = v.t_value.as_object_ptr; // The order of these two lines matters!
		cached_ptrs[t_value.as_object_ptr]++;
		break;
	case(vType::String):
		t_value.as_string_ptr = v.t_value.as_string_ptr;
		cached_ptrs[t_value.as_string_ptr]++;
		break;
	case(vType::Bool):
		t_value.as_bool = v.t_value.as_bool;
		break;
	case(vType::Double):
		t_value.as_double = v.t_value.as_double;
		break;
	case(vType::Function):
		t_value.as_function_ptr = v.t_value.as_function_ptr;
		break;
	case(vType::Integer):
		t_value.as_int = v.t_value.as_int;
		break;
	case(vType::Null):
	default: // ..What?
		break;
	}
}

//I am become death, the destroyer of malloc()
Value::~Value()
{
	switch (t_vType)
	{
	case(vType::Object):
		deref_as_obj();
		break;
	case(vType::String):
		deref_as_str();
		break;
	default:
		break;
	}
}

std::string Value::to_string() const
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
		return *(t_value.as_string_ptr);
	case(vType::Object):
		return t_value.as_object_ptr->dump();
	case(vType::Function):
		return "Function (" + t_value.as_function_ptr->get_name() +")";
	default:
		return "???";
	}
}

std::string Value::typestring() const
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
	case(vType::Function):
		return "Function";
	default:
		return "UNKNOWN!!";
	}
}

Value& Value::operator=(const Value& rhs)
{
	if (this == &Value::dev_null)
		return *this;

	if (this == &rhs)
		return *this;

	if (t_vType == rhs.t_vType && t_value.as_object_ptr == rhs.t_value.as_object_ptr)
		return *this;

	//Logic to drop the ref to a string or object already held by lhs
	switch (t_vType)
	{
	case(vType::String):
		deref_as_str();
		break;
	case(vType::Object):
		deref_as_obj();
		break;
	default:
		break;
	}

	//Logic to gain a ref to a string or object held by rhs
	//Also to actually be assigned the union's value, since "t_value = rhs.t_value" would be undefined behaviour apparently???
	switch (rhs.t_vType)
	{
	case(vType::String):
		t_value.as_string_ptr = rhs.t_value.as_string_ptr;
		cached_ptrs[rhs.t_value.as_string_ptr]++;
		break;
	case(vType::Object):
		cached_ptrs[rhs.t_value.as_object_ptr]++;
		t_value.as_object_ptr = rhs.t_value.as_object_ptr;
		break;
	case(vType::Bool):
		t_value.as_bool = rhs.t_value.as_bool;
		break;
	case(vType::Double):
		t_value.as_double = rhs.t_value.as_double;
		break;
	case(vType::Function):
		t_value.as_function_ptr = rhs.t_value.as_function_ptr;
		break;
	case(vType::Integer):
		t_value.as_int = rhs.t_value.as_int;
		break;
	case(vType::Null):
	default: // ..What?
		break;
	}
	t_vType = rhs.t_vType;
	return *this;
}
Value& Value::operator=(Value&& rhs)
{
	if (this == &Value::dev_null)
		return *this;

	//Logic to drop the ref to a string or object already held by lhs
	switch (t_vType)
	{
	case(vType::String):
		deref_as_str();
		break;
	case(vType::Object):
		deref_as_obj();
		break;
	default:
		break;
	}
	t_vType = rhs.t_vType;
	switch(t_vType)
	{
	case(vType::String):
		t_value.as_string_ptr = rhs.t_value.as_string_ptr;
		break;
	case(vType::Object):
		t_value.as_object_ptr = rhs.t_value.as_object_ptr;
		break;
	case(vType::Bool):
		t_value.as_bool = rhs.t_value.as_bool;
		break;
	case(vType::Double):
		t_value.as_double = rhs.t_value.as_double;
		break;
	case(vType::Function):
		t_value.as_function_ptr = rhs.t_value.as_function_ptr;
		break;
	case(vType::Integer):
		t_value.as_int = rhs.t_value.as_int;
		break;
	case(vType::Null):
	default: // ..What?
		break;
	}
	rhs.t_vType = vType::Null;
	rhs.t_value.as_int = 0;
	return *this;
}
/*
This exists pretty much exclusively for internal use, and is incomparable to the implementation of the == operator within Jo�o itself.
DO NOT SWALLOW OR SUBMERGE IN ACID
*/
bool Value::operator==(const Value& rhs) const
{
	if (t_vType != rhs.t_vType)
	{
		return false;
	}
	switch (t_vType)
	{
	case(vType::Bool):
		return t_value.as_bool == rhs.t_value.as_bool; // Lustfully desiring for an XNOR operator
	case(vType::Double):
		return t_value.as_double == rhs.t_value.as_double;
	case(vType::Integer):
		return t_value.as_int == rhs.t_value.as_int;
	case(vType::Null): // Null == Null ; using Null as a key would 100% never work ever otherwise
		return true;
	case(vType::Object):
		return t_value.as_object_ptr == rhs.t_value.as_object_ptr;
	case(vType::String):
		return t_value.as_string_ptr == rhs.t_value.as_string_ptr;
	case(vType::Function):
		return t_value.as_function_ptr == rhs.t_value.as_function_ptr;
	default: //what?
		return false;
	}
}