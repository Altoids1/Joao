#pragma once
#include "Forward.h"

/*
This class exists primarily for variable names,
where it is actually a very good idea to store a prehashed, low-overhead chunk of data
instead of std::string.
*/
struct ImmutableString
{
	const char* data = nullptr; // This is a C string on the heap.
	size_t precomputed_hash;
	bool heap;

	// ASSUMES THE C-STRING GIVEN ISN'T FROM THE HEAP
	ImmutableString(const char* c_str)
		:data(c_str)
		,heap(false)
		,precomputed_hash(std::hash<std::string>()(c_str))
	{

	}
	ImmutableString(const std::string& str)
		:data(strncpy(new char[str.size() + 1],str.c_str(), str.size()+1)) // +1 to include the null character
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
			data = strncpy(new char[strlen(other.data)+1], other.data, strlen(other.data) + 1);
		}
		else
		{
			data = other.data;
		}
	}
	ImmutableString& operator=(const ImmutableString& other)
	{
		precomputed_hash = other.precomputed_hash;
		heap = other.heap;
		if (data && heap)
		{
			//std::cout << "Deleting '" << data << "' in a funny way....\n";
			delete[] data;
		}
		if (heap) // We make no attempt to ref-count so... just duplicate
		{
			data = strncpy(new char[strlen(other.data) + 1], other.data, strlen(other.data) + 1);
		}
		else
		{
			data = other.data;
		}
		return *this;
	}

	ImmutableString(ImmutableString&& other)
		:data(other.data)
		,heap(other.heap)
		,precomputed_hash(other.precomputed_hash)
	{
		other.data = nullptr;
		heap = false;
	}

	~ImmutableString()
	{
		if (heap) // FIXME: We can probably use funky template madness to get rid of this if-statement and allow for trivial destruction in heapless cases.
		{
			//if(data)
				//std::cout << "Deleting '" << data << "'....\n";
				//std::cout << reinterpret_cast<size_t>(data) << " deleted.\n";
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