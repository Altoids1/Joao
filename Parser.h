#pragma once

#include "AST.h"
#include "Object.h"


class Program // this is, especially for now, just a wrapper for a master function which constitutes main()
{
	std::unordered_map<std::string, Function> definedFunctions;
	std::unordered_map<std::string, ObjectType> definedObjTypes;
	Interpreter* myinterp;
public:
	Program()
	{

	}
	Program(Function& f)
	{
		definedFunctions["main"] = f;
	}
	void set_interp(Interpreter& interp)
	{
		myinterp = &interp;
	}

	// I wanna point out that this is distinct from Interpreter's version of this function; it's a raw call to a function's name, directory data and all, while Interpreter's resolves scope first.
	Function get_func(std::string name)
	{
		assert(definedFunctions.count(name));
		return definedFunctions[name];
	}
	void set_func(std::string name, Function& f)
	{
		definedFunctions[name] = f;
	}

};

class Parser
{
	Program t_program;
};