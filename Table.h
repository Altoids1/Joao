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
	//The worker function for at_ref, at_set, and at_set_raw.
	//Allocates space within the table for the key-value pair corresponding to the 1st and 2nd args, respectively.
	//Returns a reference to the place it allocated. Must succeed.
	Value& talloc(Value, const Value&);
public:
	//Deallocates the index provided, if it exists somewhere.
	void tfree(const Value&);

	std::vector<Value> t_array;
	std::unordered_map<size_t, Value> t_hash;
	Table(std::string objty, std::unordered_map<std::string, Value>* puh, std::unordered_map<std::string, Function*>* fuh)
		:Object(objty,puh,fuh,nullptr)
	{

	}

	//Returns the value stored at this index, or NULL and a runtime if it cannot be found.
	Value at(Interpreter&, Value);
	
	//Gets a handle to the value pointed to by this index. Quietly creates a reference to a null Value if it cannot be found.
	Value& at_ref(Interpreter&, Value);

	//Sets the value pointed to by index to the value referenced by the second.
	void at_set(Interpreter&, Value, Value&);
	bool at_set_raw(Value, Value&);

	size_t length() { return t_array.size(); }
	bool virtual is_table() override { return true; }
};