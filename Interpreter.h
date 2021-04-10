#pragma once

#include "AST.h"
#include "Parser.h"

class Interpreter
{
	Program prog;
	std::string scope;
public:
	Interpreter();

	Value execute(Program&);
	Function* get_func(std::string);
	void RuntimeError(ASTNode& a,std::string what)
	{
		//This is just a basic setup while everything else is fleshed out.
		std::cout <<"ERROR: " <<what;
		//exit(1);
	}
};