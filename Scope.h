#pragma once
#include "Forward.h"

template <typename _Ty>
class Scope {
	/*
	This class can serve two functionalities, whose implementations are merged into one powerful class.
	1. This can be used by the Parser to keep track of the Object or Function tree system, to be later collapsed into Program::definedFunctions and Program::definedObjTypes, if it wants
	2. This can be used by the Interpreter to keep track of scoped variables (so, Values), allowing for their access and dismissal as it enters and exits various Scopes.
	*/
	struct Scopelet
	{
		std::unordered_map < std::string, _Ty*> table;
		std::string scopename;
		Scopelet(std::string name)
		{
			scopename = name;
		}
	};


	std::forward_list<Scopelet*> stack;

	Scopelet* top_scope = nullptr; //The backmost scoplet of the stack.


	void blank_table(Scope::Scopelet* sc)
	{
		for (auto it = sc->table.begin(); it != sc->table.end(); ++it)
		{
			delete it->second; // Hopefully this works, heh.
		}
		sc->table = {};
	}

public:


	Scope(std::string sc)
	{
		Scopelet* base = new Scopelet(sc);
		top_scope = base;
		stack.push_front(base);
	}
	std::string get_name()
	{
		return stack.front()->scopename;
	}

	_Ty* get(std::string name)
	{
		for (auto it = stack.begin(); it != stack.end(); ++it)
		{
			Scopelet sc = **it;
			if (sc.table.count(name))
				return sc.table.at(name);
		}
		return nullptr; // Give up.
	}

	_Ty* get_back(std::string name)
	{
		if (top_scope->table.count(name))
			return top_scope->table.at(name);

		return nullptr;
	}

	void set(std::string name, _Ty& t)
	{
		_Ty* tuh = new _Ty(t);

		stack.front()->table[name] = tuh;
	}

	void push(std::string name = "") // Add a new stack layer
	{
		Scopelet* newsc = new Scopelet(name);

	}

	void pop() // Delete the newest stack layer
	{
		if (top_scope == stack.front()) // Attempting to delete the base stack
		{
			std::cout << "WEIRD_ERROR: Attempted to delete backmost stack of a Scope!";
			return; // fails.
		}

		Scopelet* popped = stack.front();

		blank_table(popped);
		delete popped;

		stack.pop_front();
	}

	/*
	So this function is kinda interesting.
	The default structure of this thing is to be used as a sort of "linked list" that does these (expensive!) recursive calls to find things.
	This function allows the conversion of the linked list into one big, fat hashtable that stores everything in strings that have a directory structure,
	with the names of each directory being the Scope::scopename things of each Scope.
	*/
	std::unordered_map<std::string, _Ty*>* squish()
	{

	}
};