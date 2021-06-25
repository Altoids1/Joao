#include "Table.h"
#include "Interpreter.h"

Value Table::at(Interpreter& interp, Value index)
{
	Value::JoaoInt array_index;

	switch (index.t_vType)
	{
	default:
		interp.RuntimeError(nullptr, "Bad type used to index into Table!");
		return Value();
	case(Value::vType::String): // Just use our properties, innit?
		return get_property(interp, *index.strget());
	case(Value::vType::Integer):
		array_index = index.t_value.as_int;
		break;
	case(Value::vType::Double): //Dude. You can't use a raw-ass double to index. That's fucking stupid. You're stupid.
		//I'm rounding this while glaring at you sternly.
		array_index = math::round({ index }).t_value.as_int;
		break;
	}
	//Should be able to safely assert by this point that array_index has been set to something.

	if (array_index < 0 || array_index >= static_cast<Value::JoaoInt>(t_array.size()))
	{
		interp.RuntimeError(nullptr, "Index was out-of-bounds of array!");
		return Value();
	}

	return t_array[array_index];
}

void Table::at_set(Interpreter& interp, Value index, const Value& newval)
{
	Value::JoaoInt array_index;

	switch (index.t_vType)
	{
	default:
		interp.RuntimeError(nullptr, "Bad type used to index into Table!");
		return;
	case(Value::vType::String): // Just use our properties, innit?
	{
		//Should be analogous to set_property, except it *makes no check as to whether this property is something our ObjectType has*!!
		std::string str = *index.strget();
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

	if (array_index >= static_cast<Value::JoaoInt>(t_array.size()))
	{
		return;
		t_array.resize(static_cast<size_t>(array_index) + 1,Value()); // FIXME: this is fucking crazy and needs to be changed so as to better support sparsely-populated arrays
	}
	t_array[array_index] = newval;
}