#pragma once

#include "Forward.h"
#include "AST.h"

class Object {

public:
	std::string object_type; // A string denoting the directory position of this object.
	
	std::unordered_map<std::string, Value> properties; // A general container for all Value-type properties of this object. Functions are stored in the Program's ObjectTree.
	std::unordered_map<std::string, Value>* base_properties; //A pointer to our ObjectType's property hashtable, to look up default values when needed
	std::unordered_map<std::string, Function>* base_funcs; // Pointer to object's base functions.

	Value get_property(std::string name, Interpreter& interp)
	{

	}
	std::string dump()
	{
		std::string st = object_type + "/{";

		for (auto& it : properties) {
			// Do stuff
			st += "(" + it.first + "," + it.second.to_string() + ") ";
		}

		return st + "}";
	}
};

class ObjectType // Stores the default methods and properties of this type of Object.
//Note that this logic assumes that inheritence has already been "figured out," meaning it makes no attempt to parse some grand Object tree to figure out what to do;
//it either has the property, or it doesn't, or it has the function, or it does not.
{
	std::string object_type;
	std::unordered_map<std::string, Function> typefuncs;
	std::unordered_map<std::string, Value> typeproperties;
public:

	Object makeObject(Interpreter& interp)
	{
		Object o;
		o.base_properties = &typeproperties;
		o.base_funcs = &typefuncs;
		if (typefuncs.count("__CONSTRUCTOR"))
		{
			//typefuncs["__CONSTRUCTOR"].execute(); //TODO: Arguments for functions
		}
		return o;
	}
};