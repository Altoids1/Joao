#include "GarbageOverseer.h"
#include "Object.h"

void GarbageOverseer::add_ref(Object* o)
{
	if (!tracker.count(o))
	{
		std::cout << "Adding object: " << o->dump() << std::endl;
		tracker[o] = 1;
	}
	else
	{
		tracker.at(o)++;
	}
}
void GarbageOverseer::add_ref(std::string* s)
{
	if (!str_tracker.count(s))
	{
		std::cout << "Adding std::string: " << *s << std::endl;
		str_tracker[s] = 1;
	}
	else
	{
		str_tracker.at(s)++;
	}
}


void GarbageOverseer::remove_ref(Object* o)
{
	if (!tracker.count(o))
	{
		std::cout << "WEIRD_ERROR: Attempted to remove ref of object already deleted!\n";
		exit(1);
	}
	int& count = tracker.at(o);
	--count;
	if (!count)
	{
		std::cout << "Deleting object: " << o->dump() << std::endl;
		tracker.erase(o);
		delete o;
	}
}
void GarbageOverseer::remove_ref(std::string* s)
{
	if (!str_tracker.count(s))
	{
		std::cout << "WEIRD_ERROR: Attempted to remove ref of std::string already deleted:\n" << *s;
		exit(1);
	}
	int& count = str_tracker.at(s);
	--count;
	if (!count)
	{
		std::cout << "Deleting string: " << *s << std::endl;
		str_tracker.erase(s);
		delete s;
	}
}