#pragma once

#include "Directory.h"
#include "AST.h"
#include "Program.h"
#include "Scope.h"

class Interpreter
{
	Program* prog;

	//The global scope of variables.
	Scopelet<Value> globalscope;

	//Part of the function stack; the object associated with the current function/method. A nullptr indicates that it's a global (classless) function.
	std::stack<Object*> objectscope;

	//The scopes of each block of each function in the function stack.
	std::stack<Scope<Value>*> blockscope;
public:
	int BREAK_COUNTER = 0; // An integer flag used to break (perhaps several levels) out of one or several blocks (which are not Function blocks)
	bool FORCE_RETURN = false; // A flag used to allow blocks to force their parent functions to return when they hit a ReturnStatement.

	Interpreter();

	//Executes the given program. Assumes program already knows about it's parent interp.
	Value execute(Program&);

	//Gets function by Directory name.
	Function* get_func(std::string funkname, ASTNode* caller, bool loud = true);

	///Blockscope/omniscope

	//Initializes variable at the lowest blockscope available.
	void init_var(std::string, Value, ASTNode*);
	//Override the value of an already-existing variable at the lowest scope available.
	void override_var(std::string, Value, ASTNode*);
	//Get variable at the lowest scope available.
	Value get_var(std::string, ASTNode*);
	bool has_var(std::string, ASTNode*);

	///Objectscope
	Value get_property(std::string, ASTNode*);
	void set_property(std::string, Value, ASTNode*);
	Value grand_property(unsigned int, std::string, ASTNode*);
	Handle grand_handle(unsigned int, std::string, ASTNode*);

	//Construct an object and return it as a Value.
	Value makeObject(std::string,std::vector<ASTNode*>&,ASTNode*);

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

	//Pushes a new blockstack and objstack layer
	void push_stack(std::string name = "", Object* obj = nullptr)
	{
		blockscope.push(new Scope<Value>(name));
		objectscope.push(obj);
	}

	//Pops both the blockstack and objstack layer
	void pop_stack()
	{
		Scope<Value>* scuh = blockscope.top();
		blockscope.pop();
		delete scuh;

		objectscope.pop();
	}


	Value get_global(std::string name, ASTNode* getter)
	{
		if (!globalscope.table.count(name))
			RuntimeError(getter, "Unable to access global value: " + name);
		return *globalscope.table.at(name);
	}
	void set_global(std::string name, Value& val, ASTNode* setter)
	{
		globalscope.table[name] = new Value(val);
	}

	void push_block(std::string name = "")
	{
		Scope<Value>* scuh = blockscope.top();
		scuh->push(name);
	}
	void pop_block()
	{
		Scope<Value>* scuh = blockscope.top();
		scuh->pop();
	}
};