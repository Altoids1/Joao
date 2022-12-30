#include "Table.h"
#include "Interpreter.h"

//Should be identical to at_ref except that it returns Value() instead of doing a talloc() call.
Value Table::at(Interpreter& interp, Value index) // Intentional copy of 2nd param
{

	switch (index.t_vType)
	{
	default:
		interp.RuntimeError(nullptr, ErrorCode::BadMemberAccess, "Bad type used to index into Table!");
		return Value();
	case(Value::vType::String): // Just use our hashtable, innit?
	{
		if (t_hash.count(index))
		{
			return t_hash.at(index);
		}
		return Value();
	}
	case(Value::vType::Integer):
		break;
	case(Value::vType::Double): //Dude. You can't use a raw-ass double to index. That's fucking stupid. You're stupid.
		//I'm rounding this while glaring at you sternly.
		index = math::round({ index });
		break;
	}
	//Should be able to safely assert by this point that array_index has been set to something.

	Value::JoaoInt array_index = index.t_value.as_int;
	if (array_index < 0 || array_index >= static_cast<ptrdiff_t>(t_array.size())) // This fluke of integer casting means that arrays cannot be more than 8 exabytes big.
	{
		if (t_hash.count(index))
		{
			return t_hash.at(index);
		}
		return Value();
	}

	return t_array.at(array_index); // Friendly reminder that std::vector::at() does bounds-checking, while std::vector::operator[] does not.
}

Value& Table::at_ref(Interpreter& interp, Value index) // Intentional copy of 2nd param
{
	switch (index.t_vType)
	{
	default:
		interp.RuntimeError(nullptr, ErrorCode::BadMemberAccess, "Bad type used to index into Table!");
		return Value::dev_null;
	case(Value::vType::String): // Just use our properties, innit?
	{
		if (t_hash.count(index))
		{
			return t_hash.at(index);
		}
#ifdef JOAO_SAFE
		++interp.value_init_count;
		if (interp.value_init_count > MAX_VARIABLES)
		{
			throw error::max_variables(std::string("Program reached the limit of ") + std::to_string(MAX_VARIABLES) + std::string("instantiated variables!"));
		}
#endif
		return talloc(index, Value());
	}
	case(Value::vType::Integer):
		break;
	case(Value::vType::Double): //Dude. You can't use a raw-ass double to index. That's fucking stupid. You're stupid.
		//I'm rounding this while glaring at you sternly.
		index = math::round({ index }).t_value.as_int;
		break;
	}
	Value::JoaoInt array_index = index.t_value.as_int;

	if (array_index < 0 || array_index >= static_cast<ptrdiff_t>(t_array.size()))
	{
		if (t_hash.count(index))
		{
			return t_hash.at(index);
		}
#ifdef JOAO_SAFE
		++interp.value_init_count;
		if (interp.value_init_count > MAX_VARIABLES)
		{
			throw error::max_variables(std::string("Program reached the limit of ") + std::to_string(MAX_VARIABLES) + std::string("instantiated variables!"));
		}
#endif
		return talloc(index, Value());
	}

	return t_array.at(array_index); // Friendly reminder that std::vector::at() does bounds-checking, while std::vector::operator[] does not.
}

bool Table::at_set_raw(Value index, const Value& newval)
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
	

	switch (index.t_vType)
	{
	case(Value::vType::String):
	{
		// Unfortunately we can't just use our properties since, since these elements are removable,
		// this would allow Objects which inherit from /table to rescind their properties and go AWOL from the OOP structure,
		// which, while pleasantly chaotic, would be a horrible paradigm to allow, and honestly would probably end up being kinda buggy.

		t_hash[index] = newval;
		return t_hash.at(index);
	}
	case(Value::vType::Integer):
		break;
	case(Value::vType::Double): // It looks like you were trying to index by doubles. Did you mean to use Integers?
		index = math::round({ index });
		break;
	default: UNLIKELY // This is a bug, but, whatever.
		index = Value(0);
	}

	Value::JoaoInt array_index = index.t_value.as_int;
	if (array_index < 0)
	{
		t_hash[index] = newval;
		return t_hash.at(index);
	}

	//Would this fit in the next sequential spot?
	if (array_index == static_cast<ptrdiff_t>(t_array.size()))
	{ // Perfect! A pleasant and surprisingly common case.
		t_array.push_back(newval);
		return t_array.at(array_index);
	}

	//Is this index already in the array to begin with?
	if (array_index < static_cast<ptrdiff_t>(t_array.size()))
	{
		return t_array.at(array_index) = newval;
	}

	//..Is this index already in the HASHTABLE to begin with?
	if (t_hash.count(index))
	{
		return t_hash.at(index) = newval;
	}

	//If we won't cause a rehash by adding this new element to the hashtable...
	//FIXME: This sucks, and is likely going to be hilariously inaccurate with this novel hashtable implementation.
	//if (static_cast<float>((t_hash.size() + 1)) / t_hash.bucket_count() < t_hash.max_load_factor())
	if (static_cast<float>(t_hash.size()) / t_hash.capacity() > 0.8)
	{
		// Just stick it in there, then, tbh
		t_hash[index] = newval;
		return t_hash.at(index);
	}

	//If we *would*, do a check to make sure that we can't just move some stuff over

	for (Value::JoaoInt i = t_array.size(); i < array_index; ++i)
	{
		if (!t_hash.count(Value(i)))
			goto JUST_HASH_IT;
	}
	//Oh god, we can actually do this.
	for (Value::JoaoInt i = t_array.size(); i < array_index; ++i)
	{
		Value ival = Value(i);
		t_array.push_back(std::move(t_hash.at(ival)));
		t_hash.erase(ival);
	}
	t_array.push_back(newval);
	return t_array.at(array_index);

JUST_HASH_IT:
	t_hash[index] = newval;
	return t_hash.at(index);
}

void Table::tfree(const Value& index)
{
	//NOTE: This function should *always* follow the pattern of data access that talloc() does, since it needs to reverse-engineer what it did from the index.
	Value::JoaoInt array_index;

	switch (index.t_vType)
	{
	case(Value::vType::String):
	{
		// Unfortunately we can't just use our properties since, since these elements are removable,
		// this would allow Objects which inherit from /table to rescind their properties and go AWOL from the OOP structure,
		// which, while pleasantly chaotic, would be a horrible paradigm to allow, and honestly would probably end up being kinda buggy.

		t_hash.erase(index);
		return;
	}
	case(Value::vType::Integer):
		array_index = index.t_value.as_int;
		break;
	case(Value::vType::Double): // It looks like you were trying to index by doubles. Did you mean to use Integers?
		array_index = math::round({ index }).t_value.as_int;
		break;
	default: UNLIKELY // This is a bug, but, whatever.
		array_index = 0;
	}

	Value array_val = Value(array_index);

	if (array_index < 0)
	{
		t_hash.erase(array_val);
		return;
	}

	//Is this index already in the array to begin with?

	const Value::JoaoInt arrsize = static_cast<Value::JoaoInt>(t_array.size());
	if (array_index < arrsize)
	{
		//Oh, christ.
		
		//So right now, tfree() is only called by /table/remove()
		//So we can avoid having to do some hashtable weirdness in this case,
		//and just do an "oops it just erases it and shifts the elements afterwards! lol" for now

		//Before we do this, imagine that there's an index [9] present in the hashtable, and the array only goes from [0] to [8].
		//Inserting na�vely in that case would clobber the 9th element, which is a bad.
		//talloc() tries to prevent this, but it's still possible, so lets do some checks first.

		//FIXME: This nonsense shouldn't really be in tfree(); it'd be better as something that insert() and remove() have sovereignly,
		//so this function can be used more generically for freeing table data.
		for (Value next_key = Value(t_array.size()); t_hash.count(next_key); next_key = Value(t_array.size())) // If this is the case
		{ // take that element and actually put it on the fucking array, I guess
			t_array.push_back(std::move(t_hash.at(next_key)));
			t_hash.erase(next_key);
		}
		/*
		if (array_index + 1 == arrsize)
		{
			t_array.pop_back();
			return;
		}
		*/
		t_array.erase(t_array.begin()+array_index); // This is kind of a FIXME? Maybe? I guess?
		return;
	}

	//..Is this index already in the HASHTABLE to begin with?
	if (t_hash.count(array_val))
	{
		t_hash.erase(array_val);
		return;
	}

	//This index didn't exist. Weird.
#ifdef _DEBUG
	std::cout << "DEBUG: Attempted to tfree() an index that didn't exist!\n";
#endif
	return;
}