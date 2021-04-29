#pragma once

#include "Forward.h"
#include "AST.h"

class Object 
{
	std::unordered_map<std::string, Value> properties; // A general container for all Value-type properties of this object. Functions are stored in the Program's ObjectTree.
	std::unordered_map<std::string, Value>* base_properties; //A pointer to our ObjectType's property hashtable, to look up default values when needed
	std::unordered_map<std::string, Function*>* base_funcs; // Pointer to object's base functions.
public:
	std::string object_type; // A string denoting the directory position of this object.
	Value get_property(Interpreter&, std::string);
	void set_property(Interpreter&, std::string, Value);
	Value call_method(Interpreter&, std::string, std::vector<Value>& args);

	//Attempts to queue this object for garbage collection. TODO: Make garbage collection for objects exist.
	void qdel();

	Object(std::string objty, std::unordered_map<std::string, Value>* puh, std::unordered_map<std::string, Function*>* fuh)
		:base_properties(puh),
		base_funcs(fuh),
		object_type(objty)
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
	
	friend class ObjectType;
};

class ObjectType // Stores the default methods and properties of this type of Object.
//Note that this logic assumes that inheritence has already been "figured out," meaning it makes no attempt to parse some grand Object tree to figure out what to do;
//it either has the property, or it doesn't, or it has the function, or it does not.
{
	std::string object_type;
	std::unordered_map<std::string, Function*> typefuncs;
	std::unordered_map<std::string, Value> typeproperties;
public:
	ObjectType(std::string n)
		:object_type(n)
	{

	}
	ObjectType(std::string n, std::unordered_map<std::string, Value> typep)
		:object_type(n)
		,typeproperties(typep)
	{

	}

	//Note that this does not create a Value with Objectptr type; this is moreso an interface for the Interpreter during Object construction than anything else
	Object* makeObject(Interpreter& interp, std::vector<Value> &args)
	{
		Object* o = new Object(object_type,&typeproperties, &typefuncs); // FIXME: Make these instantiated objects capable of being garbage-collected; this is a memory leak right now!
		if (typefuncs.count("#constructor"))
		{
			Function* fuh = typefuncs["#constructor"]; //fuh.
			if(args.size())
				fuh->give_args(args, interp);
			fuh->resolve(interp);
		}
		return o;
	}


	//Passed Parser-by-reference as the Interpreter should never be calling this; ObjectTypes are static at runtime (for now, anyways!)
	void set_typeproperty(Parser&,std::string, Value);
	void set_typemethod(Parser&, std::string, Function*);
};