#pragma once
#include "Forward.h"

template <typename _Tysc>
struct Scopelet
{
	std::unordered_map < std::string, _Tysc*> table;
	std::string scopename;
	Scopelet()
	{

	}
	Scopelet(const std::string& name)
	{
		scopename = name;
	}
};

template <typename _Ty>
class Scope {
	/*
	This class can serve two functionalities, whose implementations are merged into one powerful class.
	1. This can be (but isn't) used by the Parser to keep track of the Object or Function tree system, to be later collapsed into Program::definedFunctions and Program::definedObjTypes, if it wants
	2. This can be (and is) used by the Interpreter to keep track of scoped variables (so, Values), allowing for their access and dismissal as it enters and exits various Scopes.
	*/

	std::forward_list<Scopelet<_Ty>*> stack;

	Scopelet<_Ty>* top_scope = nullptr; //The backmost scoplet of the stack.

public:
	static void blank_table(Scopelet<_Ty>* sc)
	{
		for (auto it = sc->table.begin(); it != sc->table.end(); ++it)
		{
			delete it->second; // Hopefully this works, heh.
		}
		sc->table = {};
	}

	Scope(const std::string& sc)
	{
		Scopelet<_Ty>* base = new Scopelet<_Ty>(sc);
		top_scope = base;
		stack.push_front(base);
	}
	std::string get_name()
	{
		return stack.front()->scopename;
	}

	_Ty* get(const std::string& name)
	{
		//std::cout << "The front of the stack is now " << stack.front()->scopename << "!\n";
		for (auto it = stack.begin(); it != stack.end(); ++it)
		{
			Scopelet<_Ty>& sc = **it;
			//std::cout << "Looking at scope " << sc.scopename << "...\n";

			if (sc.table.count(name))
				return sc.table.at(name);
		}
		return nullptr; // Give up.
	}

	_Ty* get_front(const std::string& name)
	{
		if (stack.front()->table.count(name))
			return stack.front()->table.at(name);
		return nullptr;
	}

	_Ty* get_back(const std::string& name)
	{
		if (top_scope->table.count(name))
			return top_scope->table.at(name);

		return nullptr;
	}
	
	std::string get_back_name()
	{
		return top_scope->scopename;
	}

	void set(const std::string& name, _Ty& t)
	{
		_Ty* tuh = new _Ty(t);

		(stack.front()->table[name]) = tuh;
	}

	bool Override(const std::string& name, _Ty& t)
	{
		for (auto it = stack.begin(); it != stack.end(); ++it)
		{
			Scopelet<_Ty>* sc = *it;
			//std::cout << "Looking at scope " << sc.scopename << "...\n";

			if (sc->table.count(name))
			{
				//FIXME: This might be a memory leak.
				_Ty* tuh = new _Ty(t);
				sc->table.erase(name);
				sc->table[name] = tuh;
				return true;
			}
		}
		return false;
	}

	void push(const std::string& name = "") // Add a new stack layer
	{
		//std::cout << "Creating new scope layer called " << name << "...\n";
		Scopelet<_Ty>* newsc = new Scopelet<_Ty>(name);
		stack.push_front(newsc);
		//std::cout << "The front of the stack is now " << stack.front()->scopename << "!\n";
	}

	void pop() // Delete the newest stack layer
	{
		//std::cout << "Stack layer " << stack.front()->scopename << " popped!\n";
		if (top_scope == stack.front()) // Attempting to delete the base stack
		{
			std::cout << "WEIRD_ERROR: Attempted to delete backmost stack of a Scope!";
			return; // fails.
		}

		Scopelet<_Ty>* popped = stack.front();

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