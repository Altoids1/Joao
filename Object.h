#pragma once

#include "Forward.h"
#include "AST.h"

class ObjectType // Stores the default methods and properties of this type of Object.
//Note that this logic assumes that inheritence has already been "figured out," meaning it makes no attempt to parse some grand Object tree to figure out what to do;
//it either has the property, or it doesn't, or it has the function, or it does not.
{
	std::string object_type;
	std::unordered_map<std::string, Function> objfuncs;
public:

};

class Object {

public:
	std::string object_type; // A string denoting the directory position of this object.
	std::unordered_map<std::string, Value> properties; // A general container for all Value-type properties of this object. Functions are stored in the Program's ObjectTree.

	Value get_property(std::string name, Interpreter& interp)
	{

	}
};