#pragma once
#include "Forward.h"
#include "Object.h"
#include "Directory.h"

/*
Tables are a very special variety of Objects, for these reasons:

1. Tables are, in part, arrays. You can do numerical indexing into them.
2. Tables are iteratable. You can iterate over their properties and array.

These things form the basis of the array-orientation of this language,
while also providing for a lovely harmonization with the object-orientation also present within this language.

For instance, every method below can (or at least, ought to be possibly) overloaded in a child type of /table,
to perhaps provide a default value to return, or other features.

This concept in general is a mixture of general OOP and Lua's concept of having a "metatable."
*/
class Table final : public Object
{
	std::vector<Value> t_array;
public:
	Table(std::string objty, std::unordered_map<std::string, Value>* puh, std::unordered_map<std::string, Function*>* fuh)
		:Object(objty,puh,fuh)
	{

	}


	Value at(Interpreter& interp, Value index)
	{
		int64_t array_index;

		switch (index.t_vType)
		{
		default:
			interp.RuntimeError(nullptr, "Bad type used to index into Table!");
			return Value();
		case(Value::vType::String): // Just use our properties, innit?
			return get_property(interp, *index.t_value.as_string_ptr);
		case(Value::vType::Integer):
			array_index = index.t_value.as_int;
			break;
		case(Value::vType::Double): //Dude. You can't use a raw-ass double to index. That's fucking stupid. You're stupid.
			//I'm rounding this while glaring at you sternly.
			array_index = math::round({ index }).t_value.as_int;
			break;
		}
		//Should be able to safely assert by this point that array_index has been set to something.

		if (array_index < 0 || array_index >= t_array.size())
		{
			interp.RuntimeError(nullptr, "Index was out-of-bounds of array!");
			return Value();
		}

		return t_array[array_index];
	}

	void at_set(Interpreter& interp, Value index, Value& newval)
	{
		int64_t array_index;

		switch (index.t_vType)
		{
		default:
			interp.RuntimeError(nullptr, "Bad type used to index into Table!");
			return;
		case(Value::vType::String): // Just use our properties, innit?
		{
			//Should be analogous to set_property, except it *makes no check as to whether this property is something our ObjectType has*!!
			std::string str = *index.t_value.as_string_ptr;
			if (properties.count(str))
			{
				properties.erase(str);
				properties[str] = newval;
			}
			else
			{
				properties[str] = newval;
			}
			return;
		}
		case(Value::vType::Integer):
			array_index = index.t_value.as_int;
			break;
		case(Value::vType::Double): // It looks like you were trying to index by doubles. Did you mean to use Integers?
			array_index = math::round({ index }).t_value.as_int;
			break;
		}

		if (array_index < 0)
		{
			interp.RuntimeError(nullptr, "Index cannot be negative in this implementation!"); //FIXME: Soon.
			return;
		}

		if (array_index >= t_array.size())
		{
			t_array.resize(array_index+1,Value()); // FIXME: this is fucking crazy and needs to be changed so as to better support sparsely-populated arrays
		}
		t_array[array_index] = newval;
	}

	size_t length() { return t_array.size(); }
	bool virtual is_table() override { return true; }
};