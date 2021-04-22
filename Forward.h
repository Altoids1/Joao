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
#define LOUD_AST // Dumps the AST before executing.

//Forwarded Objects
class Object;
class ObjectType;
class Value;
class Interpreter;