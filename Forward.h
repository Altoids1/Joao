#pragma once

//Core Libraries
#include <iostream>
#include <fstream>
#include <string>
#include <unordered_map>
#include <vector>
#include <forward_list>
#include <list>
#include <stack>

#ifdef __GNUG__
	#include <cmath>
#endif

#ifdef DEBUG
#define _DEBUG
#endif

#ifdef _DEBUG

//Debug flags
#define LOUD_SCANNER // Uncomment if you want Scanner to be verbose about what it is doing.
//#define LOUD_DEFAULT_CONSTRUCT // Uncomment if you want Token to scream every time it's default-constructed.
//#define LOUD_TOKENHEADER // Uncomment if you want every read*() function to yell about where it starts, where it ends, and how it sets the tokenheader for Parser.
#define LOUD_AST // Uncomment if you want it to dump the AST before executing it.
//#define PRINT_MAIN_RETURN_VAL // Uncomment if you want it to print the return value of main().

#endif

//#define JOAO_SAFE // Uncomment if you want Joao to be extremely suspicious of its code. Disables OS-interfacing libraries and enables throttling.
#ifdef JOAO_SAFE
#define MAX_STATEMENTS 10000 // The maximum number of statements that will be executed in JOAO_SAFE mode.
#define MAX_VARIABLES 1024 // How many variables Joao is allowed to use, as a hard limit. Includes indices.
#define MAX_RECURSION 64 // The maximum depth of recursion of Joao functions.
#endif

//Config flags
//The Interpreter has a functioning error handling system, including an ability to exit with an error code, so no #define for you.
#define EXIT_ON_PARSETIME // Uncomment if you want all parsetimes to cause João to exit(1).
//The scanner always has to crash, it can't do anything otherwise, so no #define for you.

//Version flags
#define VERSION_MAJOR 1 // For when we break || add || fix something
#define VERSION_MINOR 2 // For when we add || fix something
#define VERSION_PATCH 0 // For when we fix something
constexpr char VERSION_STRING[]{ VERSION_MAJOR + '0' , '.' , VERSION_MINOR + '0' , '.' , VERSION_PATCH + '0' , '\0'};

//Forwarded Objects
class Object;
class ObjectType;
class Metatable;

class Function;

class Table;
class Value;
class Interpreter;
class Parser;

