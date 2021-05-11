#pragma once

#include "Forward.h"
#include "AST.h"
#include "Directory.h"

class Program // this is pretty much what Parser is supposed to output, and what Interpreter is supposed to take as input.
{
	//Basically stores global functions, perhaps also static methods if we're feeling fancy.
	std::unordered_map<std::string, Function*> definedFunctions;

	//THE
	//
	//THE ENTIRE OBJECT TREE (FLATTENED)
	std::unordered_map<std::string, ObjectType*> definedObjTypes;
	Interpreter* myinterp{ nullptr };
public:
	enum class ErrorCode : int {
		NoError,
		BadArgType,
		NotEnoughArgs
	};
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
	void set_interp(Interpreter& interp)
	{
		myinterp = &interp;
	}
	void construct_natives();
	void construct_math_library();
	void construct_string_library();
	void construct_table_library();
	void construct_file_library();

	void dump()
	{
		for (auto it = definedFunctions.begin(); it != definedFunctions.end(); ++it)
		{
			Function* fuh = it->second;

			std::cout << fuh->dump(0);
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


		definedFunctions[name] = f;
	}

	
	//FIXME: I don't want Program to have any friends!! >:(
	friend class Interpreter;
	friend class Parser;
};