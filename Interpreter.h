#pragma once

#include "Directory.h"
#include "AST.h"
#include "Program.h"
#include "Scope.h"

class Interpreter
{
	Program* prog;
	std::string objscopestr;
	Scope<Value> varscope = Scope<Value>("GLOB"); // Holds globalscope and blockscope.
public:
	bool FORCE_RETURN = false; // A flag used to allow blocks to force their parent functions to return when they hit a ReturnStatement.

	Interpreter();

	//Executes the given program. Assumes program already knows about it's parent interp.
	Value execute(Program&);

	//Gets function by Directory name.
	Function* get_func(std::string, ASTNode*);

	//Set variable at the lowest blockscope available.
	void set_var(std::string, Value, ASTNode*);

	//Override the value of an already-existing variable someplace in blockscope, objectscope, or globalscope.
	void override_var(std::string, Value, ASTNode*);

	//Get variable at the lowest blockscope available.
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