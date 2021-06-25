#pragma once

#include "Forward.h"
#include "Value.h"
#include "AST.h"

class Object 
{
protected:
	std::unordered_map<std::string, Value> properties; // A general container for all Value-type properties of this object. Functions are stored in the Program's ObjectTree.
	std::unordered_map<std::string, Value>* base_properties; //A pointer to our ObjectType's property hashtable, to look up default values when needed
	std::unordered_map<std::string, Function*>* base_funcs; // Pointer to object's base functions.
	Metatable* mt = nullptr;
public:
	std::string object_type; // A string denoting the directory position of this object.

	Metatable* get_metatable() const { return mt; }

	Value* has_property(Interpreter&, std::string);
	Value get_property(Interpreter&, std::string);
	void set_property(Interpreter&, std::string, Value);

	Value call_method(Interpreter&, std::string, std::vector<Value>& args);
	Function* get_method(Interpreter&, std::string);

	//Attempts to queue this object for garbage collection. TODO: Make garbage collection for objects exist.
	void qdel();

	Object(std::string objty, std::unordered_map<std::string, Value>* puh, std::unordered_map<std::string, Function*>* fuh, Metatable* m = nullptr)
		:base_properties(puh)
		,base_funcs(fuh)
		,object_type(objty)
		,mt(m)
	{

	}

	std::string dump()
	{
		std::string st = object_type + "/{";

		for (auto& it : properties) {
			// Do stuff
			st += "(" + it.first + "," + it.second.to_string() + ") ";
		}

		return st + "}";
	}
	
	bool virtual is_table() { return false; }

	friend class ObjectType;
};

class ObjectType // Stores the default methods and properties of this type of Object.
//Note that this logic assumes that inheritence has already been "figured out," meaning it makes no attempt to parse some grand Object tree to figure out what to do;
//it either has the property, or it doesn't, or it has the function, or it does not.
{
	std::string object_type;
	std::unordered_map<std::string, Function*> typefuncs;
	std::unordered_map<std::string, Value> typeproperties;
	Metatable* mt = nullptr;
public:
	
	std::string get_name() const { return object_type; };
	bool is_table_type = false;
	ObjectType(std::string n)
		:object_type(n)
	{

	}
	ObjectType(std::string n, Metatable* m)
		:object_type(n)
		,mt(m)
	{

	}
	ObjectType(std::string n, std::unordered_map<std::string, Value> typep)
		:object_type(n)
		,typeproperties(typep)
	{

	}

	//Note that this does not create a Value with Objectptr type; this is moreso an interface for the Interpreter during Object construction than anything else
	Object* makeObject(Interpreter&, std::vector<Value>&);

	Value get_typeproperty(Interpreter&, std::string, ASTNode*);

	Value* has_typeproperty(Interpreter& interp, std::string str, ASTNode* getter)
	{
		if (!typeproperties.count(str))
		{
			return nullptr;
		}
		return &(typeproperties.at(str));
	}

	Function* has_typemethod(Interpreter&, std::string, ASTNode*);


	//Passed Parser-by-reference as the Interpreter should never be calling this; ObjectTypes are static at runtime (for now, anyways!)
	void set_typeproperty(Parser&,std::string, Value);
	void set_typemethod(Parser&, std::string, Function*);
	void set_typemethod_raw(std::string, Function*);

	friend class ObjectTree;
};

/*
This is a sort of metatype which provides overriding, perhaps natively-implemented behavior for certain operations and methods for ObjectTypes which have a pointer to it.
*/
class Metatable
{
	//These properties should be accessed by nobody but the metatable's metamethods themselves.
	std::vector<void*> privates;

	std::unordered_map<std::string, NativeMethod*> metamethods;

public:
	void append_method(std::string str, NativeMethod* newmethod)
	{
		metamethods[str] = newmethod;
	}

	void set_private(size_t index, void* ptr)
	{
		if (privates.size() <= index)
		{
			privates.resize(index+1, nullptr);
		}

		privates[index] = ptr;
	}
	void* get_private(size_t index)
	{
		if (index >= privates.size())
			return nullptr;

		return privates[index];
	}

	friend class Object;
	friend class ObjectType;
};