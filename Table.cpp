#include "Table.h"
#include "Interpreter.h"

Value Table::at(Interpreter& interp, Value index)
{
	Value::JoaoInt array_index;

	switch (index.t_vType)
	{
	default:
		interp.RuntimeError(nullptr, ErrorCode::BadMemberAccess, "Bad type used to index into Table!");
		return Value();
	case(Value::vType::String): // Just use our properties, innit?
		return *(has_property(interp, *index.t_value.as_string_ptr));
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
		interp.RuntimeError(nullptr, ErrorCode::BadMemberAccess, "Index was out-of-bounds of array!");
		return Value();
	}

	return t_array.at(array_index);
}

Value& Table::at_ref(Interpreter& interp, Value index)
{
	Value::JoaoInt array_index;

	switch (index.t_vType)
	{
	default:
		interp.RuntimeError(nullptr, ErrorCode::BadMemberAccess, "Bad type used to index into Table!");
		return Value::dev_null;
	case(Value::vType::String): // Just use our properties, innit?
	{
		size_t hash = std::hash<std::string>()(*index.t_value.as_string_ptr);
		if (t_hash.count(hash))
		{
			return t_hash.at(hash);
		}
		return talloc(index, Value());
	}
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
		size_t hash = std::hash<Value::JoaoInt>()(index.t_value.as_int);
		if (t_hash.count(hash))
		{
			return t_hash.at(hash);
		}
		return talloc(index, Value());
	}

	return t_array.at(array_index);
}

bool Table::at_set_raw(Value index, Value& newval)
{
	switch (index.t_vType)
	{
	default:
		return true;
	case(Value::vType::String):
	case(Value::vType::Integer):
	case(Value::vType::Double):
		talloc(index, newval);
		return false;
	}
}

void Table::at_set(Interpreter& interp, Value index, Value& newval)
{
	switch (index.t_vType)
	{
	default:
		interp.RuntimeError(nullptr, ErrorCode::BadMemberAccess, "Bad type used to index into Table!");
		return;
	case(Value::vType::String):
	case(Value::vType::Integer):
	case(Value::vType::Double):
		talloc(index, newval);
		return;
	}
}

Value& Table::talloc(Value index, const Value& newval)
{
	Value::JoaoInt array_index;

	switch (index.t_vType)
	{
	case(Value::vType::String):
	{
		// Unfortunately we can't just use our properties since, since these elements are removable,
		// this would allow Objects which inherit from /table to rescind their properties and go AWOL from the OOP structure,
		// which, while pleasantly chaotic, would be a horrible paradigm to allow, and honestly would probably end up being kinda buggy.

		size_t hash_index = std::hash<std::string>()(*index.t_value.as_string_ptr);
		t_hash[hash_index] = newval;
		return t_hash.at(hash_index);
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
		size_t hash_index = std::hash<Value::JoaoInt>()(index.t_value.as_int);
		t_hash[hash_index] = newval;
		return t_hash.at(hash_index);
	}

	//Would this fit in the next sequential spot?
	if (array_index == static_cast<Value::JoaoInt>(t_array.size()))
	{ // Perfect! A pleasant and surprisingly common case.
		t_array.push_back(newval);
		return t_array.at(array_index);
	}

	//Is this index already in the array to begin with?
	if (array_index < static_cast<Value::JoaoInt>(t_array.size()))
	{
		return t_array.at(array_index) = newval;
	}

	//..Is this index already in the HASHTABLE to begin with?
	size_t hash_index = std::hash<Value::JoaoInt>()(array_index);
	if (t_hash.count(hash_index))
	{
		return t_hash.at(hash_index) = newval;
	}

	//If we won't cause a rehash by adding this new element to the hashtable...
	if (static_cast<float>((t_hash.size() + 1)) / t_hash.bucket_count() < t_hash.max_load_factor())
	{
		// Just stick it in there, then, tbh
		t_hash[hash_index] = newval;
		return t_hash.at(hash_index);
	}

	//If we *would*, do a check to make sure that we can't just move some stuff over

	for (Value::JoaoInt i = t_array.size(); i < array_index; ++i)
	{
		size_t hash = std::hash<Value::JoaoInt>()(i);
		if (!t_hash.count(hash))
			goto JUST_HASH_IT;
	}
	//Oh god, we can actually do this.
	for (Value::JoaoInt i = t_array.size(); i < array_index; ++i)
	{
		size_t hash = std::hash<Value::JoaoInt>()(i);
		t_array.push_back(std::move(t_hash.at(hash)));
		t_hash.erase(hash);
	}
	t_array.push_back(newval);
	return t_array.at(array_index);

JUST_HASH_IT:
	t_hash[hash_index] = newval;
	return t_hash.at(hash_index);
}