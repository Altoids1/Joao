#pragma once

#include "SharedEnums.h"
#include "Directory.h"
#include "AST.h"
#include "Program.h"
#include "Scope.h"

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
	//Stores the current, most recent runtime error.
	Value error = Value();

	//Something used to handle rvalues in expressions. TODO: Should be blanked sometimes.
	Value tempvalue = Value();

	int BREAK_COUNTER = 0; // An integer flag used to break (perhaps several levels) out of one or several blocks (which are not Function blocks)
	bool FORCE_RETURN = false; // A flag used to allow blocks to force their parent functions to return when they hit a ReturnStatement.
#ifdef JOAO_SAFE
	int value_init_count = 0;
#endif

	Interpreter();
	Interpreter(Program&,bool);

	//Executes the given program. Assumes program already knows about it's parent interp.
	Value execute(Program&, Value&);

	//Gets function by Directory name.
	Function* get_func(const std::string& funkname, ASTNode* caller, bool loud = true);

	///Blockscope/omniscope

	//Initializes variable at the lowest blockscope available.
	void init_var(std::string, Value, ASTNode*);
	//Override the value of an already-existing variable at the lowest scope available.
	void override_var(std::string, Value, ASTNode*);
	//Get variable at the lowest scope available.
	Value& get_var(const std::string&, ASTNode*);
	bool has_var(std::string, ASTNode*);


	Object* get_objectscope() const { return objectscope.top(); }
	///Objectscope
	Value get_property(std::string, ASTNode*);
	void set_property(std::string, Value, ASTNode*);
	Value grand_property(unsigned int, std::string, ASTNode*);
	Value& grand_handle(unsigned int, std::string, ASTNode*);

	//Construct an object and return it as a Value.
	Value makeObject(std::string,std::vector<ASTNode*>&,ASTNode*);
	Value makeObject(std::string,std::vector<Value>&&,ASTNode*);
	
	//Constructs an empty base table.
	Value makeBaseTable();
	//Constructs an anonymous table with no derived classes and returns it.
	Value makeBaseTable( std::vector<Value>, std::unordered_map<std::string,Value>, ASTNode*);

	void RuntimeError()
	{
		std::cout << "RUNTIME_ERROR: UNKNOWN!";
		exit(1);
	}
	
	//Treated as a special, fatal error.
	void RuntimeError(ASTNode*, std::string);

	//Treated as a runtime which can be resumed from.
	void RuntimeError(ASTNode* node, ErrorCode err, const std::string&);

	//RuntimeError as called by the Throw keyword
	//This assumes that err_val has already been type-checked by someone higher up in the stack
	void RuntimeError(ASTNode* node, Value& err_val);

	void UncaughtRuntime(const Value& err);

	//Pushes a new blockstack and objstack layer
	void push_stack(std::string name = "", Object* obj = nullptr)
	{
#ifdef JOAO_SAFE
		if (blockscope.size() > MAX_RECURSION)
		{
			throw error::maximum_recursion(std::string("Program reached the limit of ") + std::to_string(MAX_RECURSION) + std::string("recursive calls!"));
		}
#endif
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

	//Like get_global but returns the pointer instead, and quietly allocates a new global when you ask for one it hasn't seen before.
	Value* has_global(std::string name, ASTNode* getter)
	{
		if (!globalscope.table.count(name))
		{
			globalscope.table[name] = new Value();
			return globalscope.table[name];
		}

		return globalscope.table.at(name);
	}

	Value& get_global(std::string name, ASTNode* getter)
	{
		if (!globalscope.table.count(name))
		{
			RuntimeError(getter, ErrorCode::BadAccess, "Unable to access global value: " + name); // Works, just returns null and yells.
			globalscope.table[name] = new Value();
			return *globalscope.table[name];
		}
			
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