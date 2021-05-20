#include "Scope.h"
#include "GarbageCollector.h"

template<typename _Ty>
void Scope<_Ty>::blank_table(Scopelet<_Ty>* sc)
{
	for (auto it = sc->table.begin(); it != sc->table.end(); ++it)
	{
		delete it->second; // Hopefully this works, heh.
	}
	sc->table = {};
}

template<>
void Scope<Value>::blank_table(Scopelet<Value>* sc)
{
	for (auto it = sc->table.begin(); it != sc->table.end(); ++it)
	{
		delete it->second; // Hopefully this works, heh.
	}
	sc->table = {};
}

template<typename _Ty>
void Scope<_Ty>::blank_table(Scopelet<_Ty>* sc, GarbageCollector& gc)
{
	for (auto it = sc->table.begin(); it != sc->table.end(); ++it)
	{
		delete it->second; // Hopefully this works, heh.
	}
	sc->table = {};
}

template<>
void Scope<Value>::blank_table(Scopelet<Value>* sc, GarbageCollector& gc)
{
	for (auto it = sc->table.begin(); it != sc->table.end(); ++it)
	{
		it->second->qdel(gc);
		delete it->second; // Hopefully this works, heh.
	}
	sc->table = {};
}