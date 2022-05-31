#pragma once

#include "Forward.h"
#include "Scope.h"
#include "AST.h"
#include "Directory.h"

class Program // this is pretty much what Parser is supposed to output, and what Interpreter is supposed to take as input.
{
	//Basically stores global functions, perhaps also static methods if we're feeling fancy.
	std::unordered_map<std::string, Function*> definedFunctions;

	//Methods. Separate from definedFunctions so that they are not in globalscope at runtime.
	//Stored as an unordered SET because their base name is not uniquely identifying; there can be a /foo/bar() and /fuck/bar() in the same program.
	std::unordered_set<Function*> definedMethods;

	Scopelet<Value> globals;

	//THE
	//
	//THE ENTIRE OBJECT TREE (FLATTENED)
	std::unordered_map<std::string, ObjectType*> definedObjTypes;
public:

	bool is_malformed = false;

	Program()
	{
		//Construct all the native functions
		//construct_natives();
	}
	Program(Function* f)
	{
		definedFunctions["/main"] = f;
		construct_natives();
	}
	std::unordered_map<std::string, ObjectType*> construct_natives();
	void construct_math_library();
	void construct_string_library();
	ObjectType* construct_table_library();
	ObjectType* construct_file_library();
	ObjectType* construct_error_library();

	void dump()
	{
		for (auto it = definedFunctions.begin(); it != definedFunctions.end(); ++it)
		{
			Function* fuh = it->second;

			std::cout << fuh->dump(0);
		}
		for (auto it = definedMethods.begin(); it != definedMethods.end(); ++it)
		{
			std::cout << (*it)->dump(0);
		}
	}

	// I wanna point out that this is distinct from Interpreter's version of this function; it's a raw call to a function's name, directory data and all, while Interpreter's resolves scope first.
	Function* get_func(std::string name)
	{
		if (!definedFunctions.count(name))
			return nullptr;
		return definedFunctions[name];
	}
	void set_func(std::string name, Function* f)
	{
		if (name.find_first_of('/') != std::string::npos)
			name = Directory::lastword(name);
#ifndef JOAO_SAFE
		if (definedFunctions.count(name))
		{
			std::cout << "WARNING: " << name << " overridden with alternate definition!";
		}
#endif
		definedFunctions[name] = f;
	}
	void set_meth(std::string name, Function* f)
	{
		if (name.find_first_of('/') != std::string::npos)
			name = Directory::lastword(name);
		definedMethods.insert(f);
	}
	
	//FIXME: I don't want Program to have any friends!! >:(
	friend class Interpreter;
	friend class Parser;
};
