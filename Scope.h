#pragma once
#include "Forward.h"
#include "HashTable.h"
#include "ImmutableString.h"

template <typename _Tysc>
struct Scopelet
{
	HashTable< ImmutableString, _Tysc> table;
	_Tysc* at(const ImmutableString& key) { return &(table.at(key));}
};

template <typename _Ty>
class Scope {
	/*
	This class can serve two functionalities, whose implementations are merged into one powerful class.
	1. This can be (but isn't) used by the Parser to keep track of the Object or Function tree system, to be later collapsed into Program::definedFunctions and Program::definedObjTypes, if it wants
	2. This can be (and is) used by the Interpreter to keep track of scoped variables (so, Values), allowing for their access and dismissal as it enters and exits various Scopes.
	*/

	std::deque<Scopelet<_Ty>> stack;
	std::string top_scope_name; // Usually the name of the function which owns this scope.
public:

	Scope(const std::string& sc)
		:top_scope_name(sc)
	{
		stack.push_front(Scopelet<_Ty>());
	}

	_Ty* get(const ImmutableString& name)
	{
		//std::cout << "The front of the stack is now " << stack.front()->scopename << "!\n";
		for (auto it = stack.begin(); it != stack.end(); it++)
		{
			_Ty* maybe = (*it).table.lazy_at(name);
			if (maybe)
				return maybe;
		}
		return nullptr; // Give up.
	}

	_Ty* get_front(const ImmutableString& name)
	{
		return stack.front().table.lazy_at(name);
	}

	_Ty* get_back(const ImmutableString& name)
	{
		return stack.back().table.lazy_at(name);
	}
	
	const ImmutableString& get_back_name() const
	{
		return top_scope_name;
	}

	void set(const ImmutableString& name, const _Ty& t)
	{
		(stack.front().table[name]) = t;
	}

	bool Override(const ImmutableString& name, _Ty& t)
	{
		for (auto it = stack.begin(); it != stack.end(); it++)
		{
			Scopelet<_Ty>& sc = *it;
			//std::cout << "Looking at scope " << sc.scopename << "...\n";

			if (sc.table.contains(name))
			{
				sc.table[name] = t;
				return true;
			}
		}
		return false;
	}

	void push(const std::string& name = "") // Add a new stack layer
	{
		//std::cout << "Creating new scope layer called " << name << "...\n";
		stack.push_front(Scopelet<_Ty>());
		//std::cout << "The front of the stack is now " << stack.front()->scopename << "!\n";
	}

	void pop() // Delete the newest stack layer
	{
		//std::cout << "Stack layer " << stack.front()->scopename << " popped!\n";
		if (stack.empty()) // Attempting to delete the base stack
		{
			std::cout << "WEIRD_ERROR: Attempted to delete backmost stack of a Scope!";
			return; // fails.
		}
		stack.pop_front();
	}
};