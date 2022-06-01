#pragma once
#include "Forward.h"
/*
This class exists primarily for variable names,
where it is actually a very good idea to store a prehashed, low-overhead chunk of data
instead of std::string.
*/
struct ImmutableString
{
	const char* data; // This is a C string on the heap.
	size_t precomputed_hash;
	const bool heap;

	// ASSUMES THE C-STRING GIVEN ISN'T FROM THE HEAP
	ImmutableString(const char* c_str)
		:data(c_str)
		,heap(false)
		,precomputed_hash(std::hash<std::string>()(c_str))
	{

	}
	ImmutableString(const std::string& str)
		:data(strcpy(new char[str.size() + 1],str.c_str())) // +1 to include the null character
		,heap(true)
		,precomputed_hash(std::hash<std::string>()(str))
	{

	}
	ImmutableString(const ImmutableString& other)
		:precomputed_hash(other.precomputed_hash)
		,heap(other.heap)
	{
		if (heap) // We make no attempt to ref-count so... just duplicate
		{
			data = strcpy(new char[strlen(other.data)], other.data);
		}
		else
		{
			data = other.data;
		}
	}
	ImmutableString(ImmutableString&& other)
		:data(other.data)
		,heap(other.heap)
		,precomputed_hash(other.precomputed_hash)
	{

	}

	~ImmutableString()
	{
		if (heap) // FIXME: We can probably use funky template madness to get rid of this if-statement and allow for trivial destruction in heapless cases.
		{
			//std::cout << "Deleting '" << data << "'....\n";
			delete[] data;
		}
	}

	bool operator==(const ImmutableString& other) const
	{
		if (data == other.data)
			return true;
		return strcmp(data, other.data) == 0;
	}
	std::string to_string() const
	{
		return std::string(data);
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