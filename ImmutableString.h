#pragma once
#include "Forward.h"

/*
This class exists primarily for variable names,
where it is actually a very good idea to store a prehashed, low-overhead chunk of data
instead of std::string.
*/
struct ImmutableString
{
	static Hashtable<const char*, size_t> cstr_to_refcount;
	const char* data = nullptr; // This is a C string, probably on the heap.
	size_t precomputed_hash;
	//This is the length NOT INCLUDING THE NULL CHARACTER
	size_t precomputed_length;
	bool heap;

	// ASSUMES THE C-STRING GIVEN ISN'T FROM THE HEAP
	ImmutableString(const char* c_str)
		:data(c_str)
		,heap(false)
		,precomputed_hash(std::hash<std::string>()(c_str))
		,precomputed_length(strlen(c_str))
	{

	}
	//Expensive to construct this, but it significantly reduces low-byte free()s down the road :-)
	ImmutableString(const std::string& str)
		:heap(true)
		,precomputed_hash(std::hash<std::string>()(str))
		,precomputed_length(str.size())
	{
		for (auto it : cstr_to_refcount) // walking through a hashtable, ugh :weary:
		{
			if (str == it.first) // Oh, this string is already in here!
			{
				data = it.first;
				cstr_to_refcount.at(data)++;
				return; // return in constructor, lol
			}
		}
		size_t len = str.size() + 1;
		data = strncpy(new char[len], str.c_str(), len);
		cstr_to_refcount[data] = 1u;
	}
	ImmutableString(const ImmutableString& other)
		:precomputed_hash(other.precomputed_hash)
		,precomputed_length(other.precomputed_length)
		,heap(other.heap)
	{
		data = other.data;
		if (heap)
		{
			cstr_to_refcount.at(data)++;
		}	
	}
	ImmutableString& operator=(const ImmutableString& other)
	{
		if (data && heap)
		{
			cstr_to_refcount.at(data)--;
			if (cstr_to_refcount.at(data) == 0)
			{
				cstr_to_refcount.remove(data);
				delete[] data;
			}
		}
		precomputed_hash = other.precomputed_hash;
		precomputed_length = other.precomputed_length;
		heap = other.heap;
		data = other.data;
		cstr_to_refcount.at(data)++;
		return *this;
	}
	ImmutableString& operator=(ImmutableString&& other)
	{
		if (data && heap)
		{
			cstr_to_refcount.at(data)--;
			if (cstr_to_refcount.at(data) == 0)
			{
				cstr_to_refcount.remove(data);
				delete[] data;
			}
		}
		precomputed_hash = other.precomputed_hash;
		precomputed_length = other.precomputed_length;
		heap = other.heap;
		data = other.data;
		other.data = nullptr;
		other.heap = false;
		return *this;
	}

	ImmutableString(ImmutableString&& other)
		:data(other.data)
		,heap(other.heap)
		,precomputed_hash(other.precomputed_hash)
		,precomputed_length(other.precomputed_length)
	{
		other.data = nullptr;
		other.heap = false;
	}

	~ImmutableString()
	{
		if (heap && data) // FIXME: We can probably use funky template madness to get rid of this if-statement and allow for trivial destruction in heapless cases.
		{
			cstr_to_refcount.at(data)--;
			if (cstr_to_refcount.at(data) == 0)
			{
				cstr_to_refcount.remove(data);
				delete[] data;
			}
		}
	}

	bool operator==(const ImmutableString& other) const
	{
		if (data == other.data)
			return true;
		if(precomputed_length != other.precomputed_length)
			return false;
		return strcmp(data, other.data) == 0;
	}
	std::string to_string() const
	{
		return std::string(data);
	}

	//This is a bit silly but whatever
	bool begins_with(const char* c_str)
	{
		int other_len = strlen(c_str);
		if(precomputed_length < other_len)
			return false;
		return strncmp(data,c_str,other_len) == 0;
	}
};

template <>
struct std::hash<ImmutableString>
{
    std::size_t operator()(const ImmutableString& k) const
    {
		return k.precomputed_hash;
    }
};