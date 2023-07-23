#pragma once
#include "Forward.h"

//The general vibe of Value, without any of the garbage collection. Used in a few places for values that are "permanent" for the lifetime of the program
class UnmanagedValue {
public:
	using JoaoInt = int;
	enum class vType : uint8_t {
		Null,
		Bool,
		Integer,
		Double,
		String,
		Object,
		Function // Functions as first-class values isn't 100% in yet and there's a lot of degenerate circumstances with this type.
	}t_vType{ vType::Null };

	union DATA{
		bool as_bool;
		JoaoInt as_int;
		double as_double;
		std::string* as_string_ptr;
		Object* as_object_ptr;
		Function* as_function_ptr;
	}t_value;

	//Constructors
	UnmanagedValue()
	{
		t_value.as_int = 0;
		t_vType = vType::Null;
	}
protected:
	UnmanagedValue(vType ty, DATA datum)
		:t_vType(ty)
		,t_value(datum)
	{

	}
public:
	UnmanagedValue(int64_t i)
	{
		t_value.as_int = static_cast<JoaoInt>(i);
		t_vType = vType::Integer;
	}
	UnmanagedValue(size_t i)
	{
		t_value.as_int = static_cast<JoaoInt>(i);
		t_vType = vType::Integer;
	}
	UnmanagedValue(int i)
	{
		t_value.as_int = i;
		t_vType = vType::Integer;
	}
	UnmanagedValue(double d)
	{
		t_value.as_double = d;
		t_vType = vType::Double;
	}
	UnmanagedValue(bool b)
	{
		t_value.as_bool = b;
		t_vType = vType::Bool;
	}
	UnmanagedValue(std::string s)
	{
		std::string* our_str = new std::string(s);
		t_value.as_string_ptr = our_str;
		t_vType = vType::String;
	}
	UnmanagedValue(Object* o)
	{
		t_value.as_object_ptr = o;
		t_vType = vType::Object;
	}
	UnmanagedValue(Function* f)
	{
		t_value.as_function_ptr = f;
		t_vType = vType::Function;
	}

	UnmanagedValue(vType vt, int errcode)
	{
		if (vt != vType::Null)
			return;

		t_value.as_int = static_cast<JoaoInt>(errcode);
	}

	explicit operator bool() const { // Q-q-quadruple keyword!!
		//std::cout << "Casting to bool...\n";
		switch (t_vType)
		{
		case(vType::Null):
			return false;
		case(vType::Bool):
			return t_value.as_bool;
		case(vType::Integer):
			return t_value.as_int;
		case(vType::Double):
			return t_value.as_double;
		default: // If it's a more complicated vType
			return true; // Just return true.
		}
	}
};

class Value : public UnmanagedValue { // A general pseudo-typeless Value used to store data within the programming language. Garbage collected.
	static Hashtable<void*,uint32_t> cached_ptrs;
	static std::unordered_set<std::string> cached_strings;

	inline void deref_as_str()
	{
#ifdef _DEBUG
		int cnt = cached_ptrs.count(t_value.as_string_ptr);
		if (!cnt)
		{
			std::cout << "Found object " << to_string() << " unregistered to Value ptr cache!\n";
			exit(1);
		}
#endif
		if (--cached_ptrs[t_value.as_string_ptr] == 0)
		{
			cached_ptrs.erase(t_value.as_string_ptr);
#ifdef LOUD_GC
			std::cout << "Deleting " << (*t_value.as_string_ptr) << std::endl;
#endif
			cached_strings.erase(*t_value.as_string_ptr);
			//delete t_value.as_string_ptr; -- Implied by cached_Strings.erase innit
		}
#ifdef _DEBUG
		else if(cached_ptrs[t_value.as_string_ptr] > 2'000'000)
		{
			std::cout << "Reference counter for" << *t_value.as_string_ptr << "was negative??\n";
			exit(1);
		}
#endif
	}
	void deref_as_obj();

	
public:
	static Value dev_null;
	//Constructors where Value behaves sensibly
	Value()
	:UnmanagedValue()
	{

	}
	Value(int64_t i)
	:UnmanagedValue(i)
	{

	}
	Value(size_t i)
	:UnmanagedValue(i)
	{

	}
	Value(int i)
	:UnmanagedValue(i)
	{

	}
	Value(double d)
	:UnmanagedValue(d)
	{

	}
	Value(bool b)
	:UnmanagedValue(b)
	{

	}
	Value(Function* f)
		:UnmanagedValue(f)
	{

	}
	Value(vType vt, int errcode)
		:UnmanagedValue(vt,errcode)
	{

	}
	//Constructors where things get a bit apeshit for the sake of sensible memory usage
	Value(const std::string& s)
	{
		if (cached_strings.count(s) == 0) // Novel string, it seems!
		{
			auto it = cached_strings.insert(s).first;
			t_value.as_string_ptr = const_cast<std::string*>(it.operator->());
			cached_ptrs.insert(t_value.as_string_ptr,1u);
		}
		else
		{
			t_value.as_string_ptr = const_cast<std::string*>(cached_strings.insert(s).first.operator->());
			cached_ptrs[t_value.as_string_ptr]++;
		}
		t_vType = vType::String;
	}
	Value(Object* o);
	
	//Jesus fucking christ I wish the copy-move-reference tricohotomy made any fucking sense in C++
	//For reference, this is the copy-constructor...
	Value(const Value&);

	//and this is the Move-constructor.
	Value(Value&& src) noexcept
		:UnmanagedValue(src.t_vType, src.t_value)
	{
		src.t_vType = Value::vType::Null;
		src.t_value.as_int = 0; // FIXME: this'll be unnecessary once NativeFunction's weird error codes are removed. :)
	}
	
	//And this is the assignment operator?? Kill me.
	Value& operator=(const Value&);
	Value& operator=(Value&&);

	bool operator==(const Value&) const; // For the love of god DO NOT CALL THIS

	//I am become death, the destroyer of malloc()
	~Value();

	std::string to_string() const;
	std::string typestring() const;
};


template <>
struct std::hash<Value>
{
	size_t operator()(const Value& x) const
	{
		switch (x.t_vType) // TODO: Maybe look into how to best carry out these hashings? For example, is hashing the Value's type also necessary?
		{
		case(Value::vType::Integer):
			return std::hash<Value::JoaoInt>()(x.t_value.as_int);
		case(Value::vType::Double):
			return std::hash<double>()(x.t_value.as_double);
		case(Value::vType::String):
			return std::hash<std::string>()(*x.t_value.as_string_ptr);
		default:
			throw std::runtime_error("Invalid hashing of Value!");
		}
	}
};

#if defined(_DEBUG) || defined(HASHTABLE_DEBUG) // FOR DEBUGGING ONLY. DO NOT SWALLOW OR SUBMERGE IN ACID
[[maybe_unused]] static std::ostream& operator<<(std::ostream& os, const Value& obj)
{
	os << obj.to_string();
	return os;
}
#endif
