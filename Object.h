#pragma once

#include "Forward.h"
#include "AST.h"
#include "Error.h"

class PolyTable
{
	struct HollowKnight
	{
	protected:
		HollowKnight(void* p, const std::type_info& i)
		:ptr(p)
		,info(i)
		{

		}
	public:
		void* ptr = nullptr;
		const std::type_info& info;
		virtual ~HollowKnight() = default;
	};
	template <typename Type>
	struct Vessel : public HollowKnight
	{
		Vessel(const Type& val)
		:HollowKnight(new Type(val), typeid(val))
		{
			static_assert(sizeof(*this) == sizeof(HollowKnight)); // Necessary for this to be stored in a std::vector properly.
			static_assert(!std::is_same<void,Type>()); // Why did you do this
		}
		Vessel()
		:HollowKnight(new Type(), typeid(Type))
		{

		}
		virtual ~Vessel() // I am become death, the destructor of worlds
		{
			delete static_cast<Type*>(ptr);
		}
		Vessel(const Vessel&) = delete; // Cannot be copied
		Vessel(Vessel&& other) // Can be moved, though, no biggie
		:HollowKnight(other.ptr,other.info)
		{
			other.ptr = nullptr;
		}
	};
public:
	HashTable<ImmutableString,HollowKnight> data;

	template <typename Type>
	Type& at(const ImmutableString& key)
	{
		HollowKnight& hk = data.at(key);
		if(hk.info != typeid(Type))
		{
			throw error::malicious("Data at key is of the incorrect type!"); // Haha, yes
		}
		return *static_cast<Type*>(hk.ptr);
	}
	template <typename Type>
	Type* lazy_at(const ImmutableString& key)
	{
		HollowKnight* hk = data.lazy_at(key);
		if(hk == nullptr)
			return nullptr;
		if(hk->info != typeid(Type))
		{
			throw error::malicious("Data at key is of the incorrect type!"); // Haha, yes
		}
		return static_cast<Type*>(hk->ptr);
	}
	template <typename Type>
	void insert(const ImmutableString& key, const Type& value)
	{
		data.insert(key,Vessel<Type>(value));
	}
	template <typename Type>
	void insert(const ImmutableString& key)
	{
		if constexpr(std::is_default_constructible<Type>())
		{
			data.insert(key,Vessel<Type>());
		}
		else
		{
			return;
		}
	}
};

/*
This is a sort of metatype which provides overriding, perhaps natively-implemented behavior for certain operations and methods for ObjectTypes which have a pointer to it.
*/
class Metatable
{
public:
	PolyTable metamethods; // by "void" I mean "NativeMethod

	template<typename Lambda>
	void append_method(ImmutableString str,const NativeMethod<Lambda>& newmethod)
	{
		metamethods.insert<NativeMethod<Lambda>>(str,newmethod);
	}

	friend class Object;
	friend class ObjectType;
};

class Object 
{
protected:
	Hashtable<ImmutableString, Value> properties; // A general container for all Value-type properties of this object. Functions are stored in the Program's ObjectTree.
	Hashtable<ImmutableString, Value>* base_properties; //A pointer to our ObjectType's property hashtable, to look up default values when needed
	Hashtable<ImmutableString, Function*>* base_funcs; // Pointer to object's base functions.
	Metatable* mt = nullptr;
	PolyTable mt_privates; //These properties should be accessed by nobody but the metatable's metamethods themselves.
public:
	ImmutableString object_type; // A string denoting the directory position of this object.

	PolyTable& get_privates() { return mt_privates; }

	Value* has_property(Interpreter&, const ImmutableString&);
	Value get_property(Interpreter&, const ImmutableString&);
	Value get_property_raw(const ImmutableString&);
	void set_property(Interpreter&, const ImmutableString&, Value);
	void set_property_raw(const ImmutableString&, Value);

	Value call_method(Interpreter&, const ImmutableString&, std::vector<Value>& args);
	Function* has_method(Interpreter&, const ImmutableString&);

	Object(const ImmutableString& objty, Hashtable<ImmutableString, Value>* puh, Hashtable<ImmutableString, Function*>* fuh, Metatable* m = nullptr)
		:base_properties(puh)
		,base_funcs(fuh)
		,object_type(objty)
		,mt(m)
	{

	}
	virtual ~Object() = default; // FIXME: make this not necessary to ensure tables are deleted.

	std::string dump()
	{
		std::string st = object_type.to_string() + "/{";

		for (auto it : properties) {
			// Do stuff
			st += "(" + it.first.to_string() + "," + it.second.to_string() + ") ";
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
	ImmutableString object_type;
	Hashtable<ImmutableString, Function*> typefuncs;
	Hashtable<ImmutableString, Value> typeproperties;
	Metatable* mt = nullptr;
	bool owns_metatable = false; // Marks whether we own the metatable pointer we have
public:
	
	std::string get_name() const { return object_type.to_string(); };
	bool is_table_type = false;
	ObjectType(std::string n)
		:object_type(n)
	{

	}
	ObjectType(const ImmutableString& n, Metatable* m, bool o = false)
		:object_type(n)
		,mt(m)
		,owns_metatable(o)
	{

	}
	ObjectType(const ImmutableString& n, Hashtable<ImmutableString, Value> typep)
		:object_type(n)
		,typeproperties(typep)
	{

	}

	//Note that this does not create a Value with Objectptr type; this is moreso an interface for the Interpreter during Object construction than anything else
	//It is presumed that whoever is calling this function will take ownership of this novel heap pointer and ensure that it be properly GC'd in the future.
	Object* makeObject(Interpreter&, std::vector<Value>&&);

	Value get_typeproperty(Interpreter&, const ImmutableString&, ASTNode*);

	Value* has_typeproperty(Interpreter& interp, const ImmutableString& str, ASTNode* getter)
	{
		if (!typeproperties.count(str))
		{
			return nullptr;
		}
		return &(typeproperties.at(str));
	}

	Function* has_typemethod(Interpreter&, const ImmutableString&, ASTNode*);


	//Passed Parser-by-reference as the Interpreter should never be calling this; ObjectTypes are static at runtime (for now, anyways!)
	void set_typeproperty(Parser&,std::string, Value);
	void set_typeproperty_raw(const ImmutableString&, Value);
	void set_typemethod(Parser&, std::string, Function*);
	void set_typemethod_raw(const ImmutableString&, Function*);


	template<typename Lambda>
	void append_native_method(ImmutableString str,const NativeMethod<Lambda>& newmethod)
	{
#ifdef _DEBUG
		if(!mt) [[unlikely]]
		{
			throw std::runtime_error("Metatable not yet provided for this ObjectType which has NativeMethods!");
		}
#endif
		mt->append_method(str,newmethod);
	}

	friend class ObjectTree;
};

