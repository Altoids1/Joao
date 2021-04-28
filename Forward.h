#pragma once

//Core Libraries
#include <iostream>
#include <fstream>
#include <string>
#include <unordered_map>
#include <vector>
#include <forward_list>
#include <list>

//Debug flags
//#define LOUD_SCANNER // Uncomment if you want Scanner to be verbose about what it is doing.
//#define LOUD_DEFAULT_CONSTRUCT // Uncomment if you want Token to scream every time it's default-constructed.
//#define LOUD_TOKENHEADER // Uncomment if you want every read*() function to yell about where it starts, where it ends, and how it sets the tokenheader for Parser.
#define LOUD_AST // Uncomment if you want it to dump the AST before executing it.

//Forwarded Objects
class Object;
class ObjectType;
class Value;
class Interpreter;
class Parser;

