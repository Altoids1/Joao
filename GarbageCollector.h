#pragma once

#include "Forward.h"
#include "AST.h"
#include "Object.h"

class GarbageCollector
{
	struct MemoryCell
	{
		Value::vType value_type;
		unsigned int uses;
	};
	std::unordered_map<void*, MemoryCell*> Cells; // Key is pointer, value is how many times that pointer is used.
public:
	void make(void* ptr, Value::vType v)
	{
		Cells[ptr] = new MemoryCell{ v,1 };
	}

	void add_ref(void* ptr, Value::vType v)
	{
		if (!Cells.count(ptr))
		{
			make(ptr, v);
			return;
		}
		Cells[ptr]->uses += 1;
	}

	//Mark that a new thing is perceiving this object/string/whatever.
	void add_ref(void* ptr)
	{
		if (!Cells.count(ptr))
		{
			std::cout << "MEMORY_ERROR: Unable to add new GC reference to unknown object!\n";
			exit(1);
		}

		Cells[ptr]->uses += 1;
	}

	//Mark that a thing has stopped perceiving this object/string/whatever.
	void remove_ref(void* ptr)
	{
		Cells[ptr]->uses -= 1;
		if (!Cells[ptr]->uses)
		{
			switch (Cells[ptr]->value_type)
			{
			case(Value::vType::String):
			{
				std::string* s_ptr = static_cast<std::string*>(ptr);
#ifdef LOUD_GC
				std::cout << "GC: Deallocating \"" << *s_ptr << "\"" << std::endl;
#endif
				
				delete s_ptr;
			}
			case(Value::vType::Object):
			{
				Object* o_ptr = static_cast<Object*>(ptr);
#ifdef LOUD_GC
				std::cout << "GC: Deallocating " << o_ptr->dump() << std::endl;
#endif
				
				delete o_ptr;
			}
			default:
				delete ptr; // This is... *supposed* to be safe.
			}
			delete Cells[ptr];
		}
	}
};