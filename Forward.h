#pragma once

//Most of the core libraries are included via a precompiled header, located elsewhere.

#ifdef __GNUG__
#include <cstring>
#include <cmath>
#include <cstdint>
#endif
#ifdef __cpp_lib_bitops
#include <bit>
#endif

#if __cplusplus > 202000L
#define LIKELY [[likely]]
#define UNLIKELY [[unlikely]]
#else
#define LIKELY
#define UNLIKELY
#endif

// Check windows
#if _WIN32 || _WIN64
#if _WIN64
#define ENVIRONMENT64
#else
#define ENVIRONMENT32
#endif
#endif

// Check GCC/Clang/whatever
#if __GNUC__
#if __x86_64__ || __ppc64__
#define ENVIRONMENT64
#else
#define ENVIRONMENT32
#endif
#endif

#include "config.h"

#ifdef DEBUG
#define _DEBUG
#endif

#ifdef _DEBUG // This is the preferred debug flag macro for this project.

//Debug flags
#define LOUD_SCANNER // Uncomment if you want Scanner to be verbose about what it is doing.
//#define LOUD_DEFAULT_CONSTRUCT // Uncomment if you want Token to scream every time it's default-constructed.
//#define LOUD_TOKENHEADER // Uncomment if you want every read*() function to yell about where it starts, where it ends, and how it sets the tokenheader for Parser.
#define LOUD_AST // Uncomment if you want it to dump the AST before executing it.
//#define LOUD_GC // Uncomment if you want all garbage collections to cause a notification.
//#define PRINT_MAIN_RETURN_VAL // Uncomment if you want it to print the return value of main().

#endif

//#define JOAO_SAFE // Uncomment if you want Joao to be extremely suspicious of its code. Disables OS-interfacing libraries and enables throttling.
#ifdef JOAO_SAFE
#define MAX_STATEMENTS 10000 // The maximum number of statements that will be executed in JOAO_SAFE mode.
#define MAX_VARIABLES 1024 // How many variables Joao is allowed to use, as a hard limit. Includes indices.
#define MAX_RECURSION 64 // The maximum depth of recursion of Joao functions.
#define MAX_REPLACEMENTS_ALLOWED 256 // The maximum number of replacements that /replace() can carry out.
#endif

//Forwarded Objects
class Object;
class ObjectType;
class Metatable;

class Function;

class Table;
class Value;
class Interpreter;
class Parser;
struct ImmutableString;
#include "HashTable.h"
template <typename K, typename V>
using Hashtable = HashTable<K,V>;