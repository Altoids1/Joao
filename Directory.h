#pragma once

/*
A namespace of helper functions for the manipulation of strings that indicate a directory, and I guess also other shit, innit?
*/
namespace Directory
{
	// Go up one directory from this directory
	std::string DotDot(std::string);

	// Get the last word token within this dir, returning emptystring if there is none and dir if there's no slash
	std::string lastword(std::string);

	bool is_base_of(std::string, std::string);
}

namespace string
{
	std::string replace_all(std::string, char, char);
}

namespace math
{
	Value round(const std::vector<Value>&);
	Value round_safe(Interpreter& interp, const std::vector<Value>&);
	//A math-"library" wrapper around whatever builtin/fallback is actually happening in this context.
	constexpr size_t popcount(size_t x)
	{
#ifdef __cpp_lib_bitops // Prefer a standard-library thing over weird builtins
		return std::popcount(x);
#elif defined(__GNUG__)
		if constexpr (sizeof(size_t) == 4) // x32
		{
			return __builtin_popcount(x);
		}
		else
		{
			return __builtin_popcountll(x);
		}
#else // Final fallback >:/
		size_t sum = 0;
		for (int b = 0; b < sizeof(size_t) * 8; ++b)
		{
			if (x & (1ull << b))
				++sum;
		}
		return sum;
#endif
	}
	//Returns the power of two that is larger than or equal to this number. Assumes x is not a power of two.
	constexpr size_t get_greater_power_of_two(size_t x)
	{
#ifdef __cpp_lib_bitops // If we can just use C++20, then do so
		return std::bit_ceil(x);
#endif
		//otherwise... *sigh*
		if (popcount(x) == 1) // If it's already a power of two
			return x; //bam
		x <<= 1; // shunt the bit over so it's slightly easier to find
		for (int b = (sizeof(size_t) * 8 - 1); b > 0; --b) // Find the first bit
		{
			if (x & (1ull << b)) // If this is the first bit
			{
				return 1ull << b; // Then that's your answer.
			}
		}
		return size_t(1) << (sizeof(size_t)*8 - 1);
	}
	constexpr size_t get_high_bit(size_t x)
	{
		constexpr size_t size_t_max_shift = sizeof(size_t) * 8 - 1;
#ifndef __cpp_lib_bitops
		for (int b = size_t_max_shift; b >= 0; --b)
		{
			if (x & (1 << b))
				return 1 << b;
		}
		return 0;
#else
		//Assumes x != 0
		return 1 << (size_t_max_shift - std::countl_zero(x));
#endif
	}
	template<typename... Args>
	//FIXME: Allow this to take a StringBuilder argument :3
	std::string concat(Args&&... args)
	{
		#ifndef __cpp_fold_expressions
			#error "The C++17 Fold Expressions feature is required for compilation."
		#endif
		std::string result;
		result.reserve(512);
		return (result + ... + args);
	}
}