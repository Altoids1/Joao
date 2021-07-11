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
public:
	std::vector<Value> t_array;
	Table(std::string objty, std::unordered_map<std::string, Value>* puh, std::unordered_map<std::string, Function*>* fuh)
		:Object(objty,puh,fuh,nullptr)
	{

	}


	Value& at(Interpreter&, Value);
	void at_set(Interpreter&, Value, Value&);

	size_t length() { return t_array.size(); }
	bool virtual is_table() override { return true; }
};