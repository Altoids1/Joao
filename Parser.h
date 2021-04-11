#pragma once

#include "AST.h"
#include "Object.h"

template <class _Ty>
class Scope {
	/*
	This class can serve two functionalities, whose implementations are merged into one powerful class.
	1. This can be used by the Parser to keep track of the Object or Function tree system, to be later collapsed into Program::definedFunctions and Program::definedObjTypes
	2. This can be used by the Interpreter to keep track of scoped variables (so, Values), allowing for their access and dismissal as it enters and exits various Scopes.
	*/
	std::unordered_map < std::string, _Ty*> ScopeTable;
	Scope<_Ty>* parent_scope = nullptr; // A higher-level scope to query against if we can't find our own stuff.
public:
	std::string scopename = "";

	Scope(std::string sc, Scope* s = nullptr)
		:scopename(sc),
		parent_scope(s)
	{

	}
	_Ty* get(std::string name)
	{
		if (ScopeTable.count(name)) // If we have it
		{
			return ScopeTable[name]; // Give it
		}
		//else...
		if (parent_scope != nullptr) // If we have a parent scope 
		{
			return parent_scope->get(name); // ask them
		}
		//else...
		return nullptr; // Give up.
	}
	void set(std::string name, _Ty* t)
	{
		ScopeTable[name] = t;
	}


	/*
	So this function is kinda interesting.
	The default structure of this thing is to be used as a sort of "linked list" that does these (expensive!) recursive calls to find things.
	This function allows the conversion of the linked list into one big, fat hashtable that stores everything in strings that have a directory structure,
	with the names of each directory being the Scope::scopename things of each Scope.
	*/
	/*
	std::unordered_map<std::string, _Ty*>* rdump()
	{
		std::unordered_map<std::string, _Ty*> master = ScopeTable;
		if (parent_scope)
		{
			std::unordered_map<std::string, _Ty*>* parent_ptr = parent_scope->rdump();
			master.insert()
		}
		return &master;
	}
	*/
};

class Program // this is pretty much what Parser is supposed to output, and what Interpreter is supposed to take as input.
{
	std::unordered_map<std::string, Function*> definedFunctions;
	std::unordered_map<std::string, ObjectType*> definedObjTypes;
	Interpreter* myinterp{ nullptr };
public:
	Program()
	{

	}
	Program(Function *f)
	{
		definedFunctions["main"] = f;
	}
	void set_interp(Interpreter& interp)
	{
		myinterp = &interp;
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
		definedFunctions[name] = f;
	}

};

class Parser
{
	Program t_program;
};