#pragma once

#include "Forward.h"
#include "SharedEnums.h"

class Value { // A general pseudo-typeless Value used to store data within the programming language.
	static std::unordered_map<Object*, std::shared_ptr<Object>*> known_objptrs; // A cache of all pointers for which we've seen before
public:
	using JoaoInt = int;
	enum class vType : uint8_t {
		Null,
		Bool,
		Integer,
		Double,
		String,
		Object
	}t_vType{ vType::Null };

	union {
		bool as_bool;
		JoaoInt as_int;
		double as_double;
	}t_value;
	std::shared_ptr<std::string> strptr;
	std::shared_ptr<Object> objptr;
	

	Object* objget() const
	{
		return objptr.get();
	}
	std::string* strget() const
	{
		return strptr.get();
	}

	//Constructors
	Value()
	{
		t_value.as_int = 0;
	}
	Value(int64_t i)
	{
		t_value.as_int = static_cast<JoaoInt>(i);
		t_vType = vType::Integer;
	}
	Value(size_t i)
	{
		t_value.as_int = static_cast<JoaoInt>(i);
		t_vType = vType::Integer;
	}
	Value(int i)
	{
		t_value.as_int = i;
		t_vType = vType::Integer;
	}
	Value(double d)
	{
		t_value.as_double = d;
		t_vType = vType::Double;
	}
	Value(bool b)
	{
		t_value.as_bool = b;
		t_vType = vType::Bool;
	}

	Value(std::string);

	Value(Object*);

	Value(Value::vType vt, int errcode)
	{
		if (vt != Value::vType::Null)
			return;

		t_value.as_int = static_cast<JoaoInt>(errcode);
	}

	explicit operator bool() const { // Q-q-quadruple keyword!!
		//std::cout << "Casting to bool...\n";
		switch (t_vType)
		{
		case(vType::Null):
			return false;
		case(vType::Bool):
			return t_value.as_bool;
		case(vType::Integer):
			return t_value.as_int;
		case(vType::Double):
			return t_value.as_double;
		default: // If it's a more complicated vType
			return true; // Just return true.
		}
	}


	std::string to_string();
	std::string typestring();
};