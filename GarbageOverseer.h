#pragma once
#include "Forward.h"

class GarbageOverseer
{
	std::unordered_map<Object*, int> tracker;
	std::unordered_map<std::string*, int> str_tracker;

public:
	void add_ref(Object*);
	void add_ref(std::string*);
	void remove_ref(Object*);
	void remove_ref(std::string*);
};