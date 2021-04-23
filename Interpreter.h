#pragma once

#include "AST.h"
#include "Program.h"
#include "Scope.h"

class Interpreter
{
	Program* prog;
	std::string objscopestr;
	Scope<Value> varscope = Scope<Value>("GLOB");
public:
	bool FORCE_RETURN = false; // A flag used to allow blocks to force their parent functions to return when they hit a ReturnStatement.

	Interpreter();
	Value execute(Program&);
	Function* get_func(std::string, ASTNode*);
	void set_var(std::string, Value, ASTNode*);
	Value get_var(std::string,ASTNode*);
	void RuntimeError()
	{
		std::cout << "RUNTIME_ERROR: UNKNOWN!";
		exit(1);
	}
	void RuntimeError(ASTNode* a, std::string what)
	{
		//This is just a basic setup while everything else is fleshed out.
		std::cout << "RUNTIME_ERROR: " << what << "\n";
		exit(1);
	}
	void RuntimeError(ASTNode& a, std::string what)
	{
		//This is just a basic setup while everything else is fleshed out.
		std::cout <<"RUNTIME_ERROR: " << what << "\n";
		exit(1);
	}

	void push_stack(std::string name = "")
	{
		varscope.push(name);
	}
	void pop_stack()
	{
		varscope.pop();
	}
};