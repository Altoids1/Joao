#pragma once

#include "Directory.h"
#include "AST.h"
#include "Program.h"
#include "Scope.h"
#include "GarbageOverseer.h"

class Interpreter
{
	Program* prog = nullptr;

	//The global scope of variables.
	Scopelet<Value> globalscope;

	//Part of the function stack; the object associated with the current function/method. A nullptr indicates that it's a global (classless) function.
	std::stack<Object*> objectscope;

	//The scopes of each block of each function in the function stack.
	std::stack<Scope<Value>*> blockscope;

	const bool is_interactive;
public:
	GarbageOverseer gc;

	bool did_error = false;
	int BREAK_COUNTER = 0; // An integer flag used to break (perhaps several levels) out of one or several blocks (which are not Function blocks)
	bool FORCE_RETURN = false; // A flag used to allow blocks to force their parent functions to return when they hit a ReturnStatement.

	Interpreter();
	Interpreter(Program&,bool);

	//Executes the given program. Assumes program already knows about it's parent interp.
	Value execute(Program&, Value&);

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
	
	//Constructs an empty base table.
	Value makeBaseTable();
	//Constructs an anonymous table with no derived classes and returns it.
	Value makeBaseTable( std::vector<Value>, std::unordered_map<std::string,Value>, ASTNode*);

	void RuntimeError()
	{
		std::cout << "RUNTIME_ERROR: UNKNOWN!";
#ifdef EXIT_ON_RUNTIME
		exit(1);
#endif
	}
	
	void RuntimeError(ASTNode*, std::string);


	void push_block(std::string name = "")
	{
		Scope<Value>* scuh = blockscope.top();
		scuh->push(name);
	}
	void pop_block()
	{
		/*
		We have to do something kinda dirty to make sure that any Object-referencing values are properly garbage-collected by this.
		This is partially a problem incited by the fact that Scope is a templated class and ergo can't really do the spicy with Value the way we can.
		*/
		Scope<Value>* scuh = blockscope.top();

		Scopelet<Value>* poppable = scuh->getBottomScopelet();
		const std::unordered_map<std::string, Value*>& fnuh = poppable->table;
		for (auto it = fnuh.begin(); it != fnuh.end(); ++it)
		{
			if (it->second->t_vType == Value::vType::Object)
			{
				gc.remove_ref(it->second->t_value.as_obj_ptr);
			}
			else if (it->second->t_vType == Value::vType::String)
			{
				gc.remove_ref(it->second->t_value.as_str_ptr);
			}
		}

		scuh->pop();
	}

	//Pushes a new blockstack and objstack layer
	void push_stack(std::string name = "", Object* obj = nullptr)
	{
		blockscope.push(new Scope<Value>(name));
		if (obj)
		{
			gc.add_ref(obj); // Being on the stack counts as a reference to it, I guess.
		}
		objectscope.push(obj);
	}

	//Pops both the blockstack and objstack layer
	void pop_stack()
	{
		Scope<Value>* scuh = blockscope.top();
		while (!scuh->isThin())
		{
			pop_block();
		}
		delete scuh;
		blockscope.pop();
		
		Object* top = objectscope.top();
		if(top)
		{
			gc.remove_ref(top);
		}
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

	std::string* makeString(const std::string& cpy)
	{
		std::string* ptr = new std::string(cpy);
		gc.add_ref(ptr);
		return ptr;
	}
};