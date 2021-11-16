#pragma once

//#define NDEBUG
#include <assert.h> 

#include "Forward.h"
#include "SharedEnums.h"
#include "Error.h"
#include "Value.h"

class ASTNode // ASTNodes are abstract symbols which together form a "flow chart" tree of symbols that the parser creates from the text that the interpreter then interprets.
{

public:
	int my_line = 0; // Hypothetically, the line that this ASTNode happens on. This is remembered for the sake of improving runtime legibility.

	virtual const std::string class_name() const { return "ASTNode"; }

	// Collapses this symbol into a real dang thing or process that the interpreter can do.
	virtual Value resolve(Interpreter&); 

	// Attempts to collapse this symbol in a constant and scope-less manner. Bool determines if it throws an error when this fails or not.
	virtual Value const_resolve(Parser&, bool);

	// Returns a reference to whatever Value this ASTNode points to, if any, which you can freely use the operator= on to set it to some new value.
	// Mostly to be used by AssignmentStatement and friends.
	virtual Value& handle(Interpreter&);

	virtual std::string dump(int indent) { return std::string(indent, ' ') + class_name() + "\n"; } // Used for debugging

	virtual bool is_expression() { return false; }
};

class Literal : public ASTNode { // A node which denotes a plain ol' literal.
	Value heldval;
public:
	Literal(Value V)
		:heldval(V)
	{
		//std::cout << "Constructing literal...\n";
	}

	virtual Value resolve(Interpreter&) override;
	virtual Value const_resolve(Parser&, bool) override;
	virtual std::string dump(int indent) { return std::string(indent, ' ') + "Literal: " + heldval.to_string() + "\n"; }
};

// Expressions are ASTNodes that make sense to be done on their lonesome as a statement w/o other context.
class Expression : public ASTNode
{
	static int expr_count;
protected:
#ifdef JOAO_SAFE
	void increment()
	{
		++expr_count;
		if (expr_count > MAX_STATEMENTS)
		{
			throw error::maximum_expr(std::string("Program reached the limit of ") + std::to_string(MAX_STATEMENTS) + std::string("expressions!"));
		}
	}
#endif
public:
	virtual bool is_expression() override { return true; }
};

class Identifier : public ASTNode
{
	std::string t_name;
public:
	Identifier(std::string s)
		:t_name(s)
	{
		//std::cout << "I've been created with name " + s + "!\n";
	}
	std::string get_str()
	{
		return t_name;
	}

	virtual Value resolve(Interpreter&) override;
	virtual Value& handle(Interpreter&) override;
	
	virtual const std::string class_name() const override { return "Identifier"; }
	virtual std::string dump(int indent) { return std::string(indent, ' ') + "Identifier: " + t_name + "\n"; }
};

class AssignmentStatement : public Expression
{
protected:
	ASTNode* id;
	ASTNode* rhs;
public:
	enum class aOps : uint8_t {
		NoOp, // More a Parser abstraction than anything else
		Assign, // =
		AssignAdd, // +=
		AssignSubtract, // -=
		AssignMultiply, // *=
		AssignDivide // /=
	}t_op;
	AssignmentStatement(ASTNode* i, ASTNode* r, aOps o = aOps::Assign, int linenum = 0)
		:id(i),
		rhs(r),
		t_op(o)
	{
		my_line = linenum;
		
		//std::cout << "My identifier has the name " + id->get_str() + "!\n";
	}
	virtual Value resolve(Interpreter&) override;
	virtual std::string dump(int indent)
	{ 
		std::string ind = std::string(indent, ' ');
		std::string str = ind + "AssignmentStatement, Operation: Assign\n";

		str += id->dump(indent + 1);
		str += rhs->dump(indent + 1);

		return str;
	}
};

class LocalAssignmentStatement final : public AssignmentStatement // "Value x = 3;", which has a distinct ASTNode type from "x = 3;"
{
	LocalType ty = LocalType::Value;

	bool typecheck(Value& ruh) // returns true if it passes the typecheck, false if it fails
	{
		if (ruh.t_vType == Value::vType::Null)
			return true; // Allows for null-initialization of these things

		switch (ty)
		{
		case(LocalType::Value):
			break;
		case(LocalType::Number):
			if (ruh.t_vType == Value::vType::Integer || ruh.t_vType == Value::vType::Double)
				break;
			return false;
			break;
		case(LocalType::String):
			if (ruh.t_vType == Value::vType::String)
				break;
			return false;
			break;
		case(LocalType::Boolean):
			if (ruh.t_vType == Value::vType::Bool)
				break;
			return false;
			break;
		case(LocalType::Object):
			if (ruh.t_vType == Value::vType::Object)
				break;
			return false;
			break;
		default:
			return false;
		}
		return true;
	}
public:


	LocalAssignmentStatement(Identifier* i, ASTNode* r, aOps o, LocalType localtype, int linenum = 0)
		:AssignmentStatement(i,r,o)
		,ty(localtype)
	{
		my_line = linenum;
		//std::cout << "My identifier has the name " + id->get_str() + "!\n";
	}

	virtual Value resolve(Interpreter&) override;
	virtual Value const_resolve(Parser&, bool) override;

	//A special-snowflake resolver for LocalAssignmentStatement which returns the key-value pair of the property it describes.
	std::pair<std::string, Value> resolve_property(Parser&);

	virtual std::string dump(int indent)
	{
		std::string ind = std::string(indent, ' ');
		std::string str = ind + "LocalAssignmentStatement, Type: Value, Operation: Assign\n";

		str += id->dump(indent + 1);
		str += rhs->dump(indent + 1);

		return str;
	}
};

class UnaryExpression : public Expression
{
	public:
	enum class uOps : uint8_t {
		NoOp,
		Not,
		Negate,
		BitwiseNot,
		Length
	}t_op;
	ASTNode* t_rhs;
	UnaryExpression(uOps Operator, ASTNode* r, int linenum = 0)
		:t_op(Operator)
		,t_rhs(r)
	{
		my_line = linenum;

	}
	virtual Value resolve(Interpreter&) override;
	virtual const std::string class_name() const override { return "UnaryExpression"; }
	virtual std::string dump(int indent) override
	{
		std::string ind = std::string(indent, ' ');
		std::string op;
		switch (t_op)
		{
		case(uOps::Not):
			op = "Not";
			break;
		case(uOps::BitwiseNot):
			op = "BitwiseNot";
			break;
		case(uOps::Length):
			op = "Length";
			break;
		case(uOps::Negate):
			op = "Negate";
			break;
		default:
			op = "???";
			break;
		}

		std::string str = ind + "UnaryExpresion, Operation: " + op + "\n";

		return str + t_rhs->dump(indent + 1);
	}
};

class BinaryExpression : public Expression 
{ // An ASTNode which is an operation between two, smaller Expressions.

public:
	enum class bOps : uint8_t {
		NoOp, // Used by the parser to store what is basically a null into one of these bOps enums.
		//
		Add,
		Subtract,
		Multiply,
		Divide,
		//
		FloorDivide,
		Exponent,
		Modulo,
		//
		BitwiseAnd,
		BitwiseXor,
		BitwiseOr,
		//
		ShiftRight,
		ShiftLeft,
		//
		Concatenate,
		//
		LessThan,
		LessEquals,
		Greater,
		GreaterEquals,
		Equals,
		NotEquals,
		//
		LogicalAnd,
		LogicalOr,
		LogicalXor
	}t_op;
	ASTNode* t_lhs, *t_rhs;

	BinaryExpression(bOps Operator, ASTNode* l, ASTNode* r, int linenum = 0)
		:t_op(Operator)
		,t_lhs(l) // What the fuck is this syntax holy shit
		,t_rhs(r)
	{
		my_line = linenum;
		
	}
	virtual Value resolve(Interpreter&) override;
	virtual std::string dump(int indent)
	{
		std::string ind = std::string(indent, ' ');
		std::string sop;
		switch (t_op)
		{
		case(bOps::NoOp):
			sop = "NoOp, somehow?";
			break;
		case(bOps::Add):
			sop = "Add";
			break;
		case(bOps::Subtract):
			sop = "Subtract";
			break;
		case(bOps::Multiply):
			sop = "Multiply";
			break;
		case(bOps::Divide):
			sop = "Divide";
			break;
		//
		case(bOps::Exponent):
			sop = "Exponent";
			break;
		//
		case(bOps::Concatenate):
			sop = "Concatenate";
			break;
		//
		case(bOps::ShiftLeft):
			sop = "ShiftLeft";
			break;
		//
		case(bOps::Equals):
			sop = "Equals";
			break;
		case(bOps::NotEquals):
			sop = "NotEquals";
			break;
		//

		default:
			sop = "????";
		}
		std::string str = ind + "BinaryStatement, Operation: " + sop + "\n";
		
		str += t_lhs->dump(indent + 1);
		str += t_rhs->dump(indent + 1);

		return str;
	}
	static Value BinaryOperation(Value&, Value&, bOps, Interpreter&);
};

class ReturnStatement : public Expression {
	ASTNode *held_expr;
public:
	bool has_expr{ false };

	virtual const std::string class_name() const override { return "ReturnStatement"; }

	ReturnStatement()
		:held_expr(new Literal(Value()))
	{

	}
	ReturnStatement(ASTNode* node, int linenum = 0)
		:held_expr(node)
	{
		my_line = linenum;
		has_expr = true;
	}
	virtual Value resolve(Interpreter& interp) override
	{
		return held_expr->resolve(interp);
	}
	virtual std::string dump(int indent)
	{
		std::string ind = std::string(indent, ' ');
		std::string str = ind + "ReturnStatement:\n";

		str += held_expr->dump(indent + 1);

		return str;
	}
};

class CallExpression : public Expression {
	ASTNode* func_expr;

	std::vector<ASTNode*> args;
public:
	CallExpression(ASTNode* f)
		:func_expr(f)
	{

	}
	CallExpression(ASTNode* f, std::vector<ASTNode*> arr, int linenum = 0)
		:func_expr(f)
		,args(arr) // Arr!!
	{
		my_line = linenum;

	}
	CallExpression* append_arg(ASTNode* expr)
	{
		args.push_back(expr);
		return this;
	}

	virtual Value resolve(Interpreter&) override;
	virtual Value& handle(Interpreter&) override;
	virtual const std::string class_name() const override { return "CallExpression"; }
	virtual std::string dump(int indent) override {
		std::string ind = std::string(indent, ' ');
		std::string str = ind + "CallExpression\n";
		str += ind +"@Function:\n" + func_expr->dump(indent+1);
		str += ind + "(Args:\n";
		for (size_t i = 0; i < args.size(); ++i)
		{
			str += args[i]->dump(indent + 1);
		}
		str += ind + ")\n";
		return str;
	}
};

class Function : public ASTNode
{
protected:
	Value returnValue = Value(); // My return value
	std::vector<Expression*> statements; // The statements which're executed when I am run
	std::vector<Value> t_args;
	std::vector<std::string> t_argnames;
	//std::vector<Expression> args;
	std::string t_name; // My name
	Object* obj = nullptr; // I don't know.
	Value my_value; // A value reference which represents me. of type Function; necessary for somethings sometimes

	Function()
		:my_value(Value(this))
	{

	}
public:
	
	Value& to_value() { return my_value; }
	std::string get_name() const { return t_name; }
	Object* get_obj() const { return obj; }
	void set_obj(Object* o) { obj = o; };
	Function(std::string name, Expression* expr)
		:my_value(Value(this))
	{
		statements = std::vector<Expression*>{ expr };
	}
	Function(std::string name, std::vector<Expression*> exprs) // argument-less-ness
		:my_value(Value(this))
	{
		t_name = name;
		statements = exprs;
	}
	Function(std::string name, std::vector<Expression*> &exprs, std::vector<std::string>& sargs, int linenum = 0)
		:my_value(Value(this))
	{
		t_name = name;
		statements = exprs;
		t_argnames = sargs;
		my_line = linenum;
	}
	void give_args(Interpreter&, const std::vector<Value>&, Object*);
	Function* append(Expression* expr)
	{
		statements.push_back(expr);
		return this;
	}

	virtual Value resolve(Interpreter&) override;
	virtual const std::string class_name() const override { return "Function"; }
	virtual std::string dump(int indent) override
	{
		//std::string ind = std::string(indent, ' ');
		std::string str = "Function, name: " + t_name + "\n";

		str += "@Params:\n";
		for (size_t i = 0; i < t_argnames.size(); ++i)
		{
			str += " " + t_argnames[i] + "\n";
		}

		str += "=Statements:\n";
		for (size_t i = 0; i < statements.size(); ++i)
		{
			str += statements[i]->dump(indent + 1);
		}

		return str;
	}
};

class NativeFunction final : public Function
{
	/*
	So this kinda overrides a lot of the typical behavior of a function, instead deferring it into some kickass lambda stored within.
	*/
	Value(*lambda)(std::vector<Value>) = nullptr;
	// Value lambda() {};
public:
	NativeFunction(std::string n)
		:Function()
	{
		t_name = n;
		lambda = [](std::vector<Value> args)
		{
			return Value();
		};
	}
	NativeFunction(std::string n, Value(*luh)(std::vector<Value>), int linenum = 0)
		:Function()
	{
		t_name = n;
		lambda = luh;
		my_line = linenum;
	}

	virtual Value resolve(Interpreter&) override;
	virtual const std::string class_name() const override { return "NativeFunction"; }
	virtual std::string dump(int indent) override
	{
		return "NativeFunction, name: " + t_name + "\n";
	}
};

//Similar to a NativeFunction except it requires a handle to an object its acting within.
class NativeMethod final : public Function
{
	/*
	So this kinda overrides a lot of the typical behavior of a function, instead deferring it into some kickass lambda stored within.
	*/
	Value(*lambda)(std::vector<Value>,Object*) = nullptr;
	// Value lambda() {};
public:
	bool is_static = false; // True if it actually doesn't need an object to act on
	NativeMethod(std::string n)
		:Function()
	{
		t_name = n;
		lambda = [](std::vector<Value> args,Object*)
		{
			return Value();
		};
	}
	NativeMethod(std::string n, Value(*luh)(std::vector<Value>,Object*), int linenum = 0)
		:Function()
	{
		t_name = n;
		lambda = luh;
		my_line = linenum;
	}

	virtual Value resolve(Interpreter&) override;
	virtual const std::string class_name() const override { return "NativeMethod"; }
	virtual std::string dump(int indent) override
	{
		return "NativeMethod, name: " + t_name + "\n";
	}
};

class Block : public Expression
{
protected:
	std::vector<Expression*> statements;

	Block()
	{

	}
	Block(const std::vector<Expression*>& s)
		:statements(s)
	{

	}

	//A more abstract function used for iterating over any arbitrary block of statements.
	//It's strange, but necessary for a class that is this abstract.
	Value iterate(const std::vector<Expression*>&, Interpreter&);

	//Iterate over our statements specifically.
	Value iterate_statements(Interpreter&);
};

class IfBlock final : public Block
{
	ASTNode* condition = nullptr;
	IfBlock* Elseif = nullptr;
public:
	IfBlock(std::vector<Expression*>& st)
	{
		statements = st;
	}
	IfBlock(ASTNode* cond, std::vector<Expression*>& st, int linenum = 0)
		:condition(cond)
{
		my_line = linenum;
		statements = st;
	}

	void append_else(IfBlock* elif)
	{
		if (Elseif) // If we already have an elif
			Elseif->append_else(elif); // append this to the bottom of the current chain. FIXME: Recursion is bad and you should feel bad.
		else
			Elseif = elif;
	}

	virtual Value resolve(Interpreter&) override;
	virtual const std::string class_name() const override { return "IfBlock"; }
	virtual std::string dump(int indent)
	{
		std::string ind = std::string(indent, ' ');
		std::string str = ind;
		

		if (condition)
		{
			str += "IfBlock\n";
			str += ind + "?Cond:\n" + condition->dump(indent + 2);
		}
		else
		{
			str += "ElseBlock\n";
		}

		for (size_t i = 0; i < statements.size(); ++i)
		{
			str += statements[i]->dump(indent + 1);
		}
		if (Elseif)
		{
			str += Elseif->dump(indent);
		}
		return str;
	}
};

class ForBlock final : public Block
{
	ASTNode* initializer = nullptr;
	ASTNode* condition = nullptr;
	ASTNode* increment = nullptr;
	
public:
	ForBlock(std::vector<Expression*>& st)
	{
		statements = st;
	}
	ForBlock(ASTNode* init, ASTNode* cond, ASTNode* inc, std::vector<Expression*>& st, int linenum = 0)
		:initializer(init),
		condition(cond),
		increment(inc)
	{
		my_line = linenum;
		statements = st;
	}

	virtual Value resolve(Interpreter&) override;
	virtual const std::string class_name() const override { return "ForBlock"; }
	virtual std::string dump(int indent)
	{
		std::string ind = std::string(indent, ' ');
		std::string str = ind + "ForBlock\n";
		
		str += ind + "=Init:\n" + initializer->dump(indent + 2);
		str += ind + "?Cond:\n" + condition->dump(indent + 2);
		str += ind + "+Inc:\n" + increment->dump(indent + 2);

		for (size_t i = 0; i < statements.size(); ++i)
		{
			str += statements[i]->dump(indent + 1);
		}

		return str;
	}
};

class WhileBlock final : public Block
{
	ASTNode* condition = nullptr;
public:
	WhileBlock(std::vector<Expression*>& st)
	{
		statements = st;
	}
	WhileBlock(ASTNode* cond, std::vector<Expression*>& st, int linenum = 0)
		:condition(cond)
{
		my_line = linenum;
		statements = st;
	}

	virtual Value resolve(Interpreter&) override;
	virtual const std::string class_name() const override { return "WhileBlock"; }
	virtual std::string dump(int indent)
	{
		std::string ind = std::string(indent, ' ');
		std::string str = ind + "WhileBlock\n";

		str += ind + "?Cond:\n" + condition->dump(indent + 2);

		for (size_t i = 0; i < statements.size(); ++i)
		{
			str += statements[i]->dump(indent + 1);
		}

		return str;
	}
};


class BreakStatement final : public Expression
{
	int breaknum;
public:
	BreakStatement(int br = 1, int linenum = 0)
		:breaknum(br)
{
		my_line = linenum;
	}

	virtual Value resolve(Interpreter&) override;
	virtual const std::string class_name() const override { return "BreakStatement"; }
	virtual std::string dump(int indent)
	{
		std::string ind = std::string(indent, ' ');
		std::string str = ind + "Break " + std::to_string(breaknum) + ";\n";

		return str;
	}
};

class MemberAccess final : public ASTNode
{

	ASTNode* front;
	ASTNode* back;
public:
	MemberAccess(ASTNode* f, ASTNode* b, int linenum = 0)
		:front(f)
		 ,back(b)
	{
		my_line = linenum;

	}

	virtual Value resolve(Interpreter&) override;
	virtual Value& handle(Interpreter&) override;

	virtual const std::string class_name() const override { return "MemberAccess"; }
	virtual std::string dump(int indent)
	{
		std::string ind = std::string(indent, ' ');
		std::string str = ind + "MemberAccess\n";

		str += front->dump(indent + 1);
		str += back->dump(indent + 1);

		return str;
	}



};

class ClassDefinition final : public ASTNode
{
	std::vector<LocalAssignmentStatement*> statements;
public:
	std::string direct;
	ClassDefinition(std::string& d, std::vector<LocalAssignmentStatement*> &s, int linenum = 0)
		:statements(s)
		,direct(d)
{
		my_line = linenum;

	}

	//virtual Value resolve(Interpreter&) override;

	std::unordered_map<std::string, Value> resolve_properties(Parser&);
	void append_properties(Parser&, ObjectType*);

	virtual const std::string class_name() const override { return "ClassDefinition"; }
	virtual std::string dump(int indent)
	{
		std::string ind = std::string(indent, ' ');
		std::string str = ind + "ClassDefinition " + direct + ";\n";
		for (size_t i = 0; i < statements.size(); ++i)
		{
			str += statements[i]->dump(indent + 1);
		}
		return str;
	}
};

class Construction : public ASTNode
{

	std::string type;
	std::vector<ASTNode*> args;
public:
	Construction(std::string t, std::vector<ASTNode*> a, int linenum = 0)
		:type(t)
		,args(a)
{
		my_line = linenum;

	}

	virtual const std::string class_name() const override { return "Construction"; }
	virtual std::string dump(int indent) override
	{
		std::string ind = std::string(indent, ' ');
		std::string str = std::string(indent, ' ') + "Construction, type: " + type + "\n";
		str += ind + "(Args:\n";
		for (size_t i = 0; i < args.size(); ++i)
		{
			str += args[i]->dump(indent + 1);
		}
		str += ind + ")\n";
		return str;
	}
	virtual Value resolve(Interpreter&) override;
};

class ParentAccess : public ASTNode
{
public:
	std::string prop;

	ParentAccess(std::string p, int linenum = 0)
		:prop(p)
{
		my_line = linenum;

	}
	virtual const std::string class_name() const override { return "ParentAccess"; }
	virtual std::string dump(int indent) override
	{
		return std::string(indent, ' ') + "ParentAccess, property: " + prop + "\n";
	}
	virtual Value resolve(Interpreter&) override;
	virtual Value& handle(Interpreter&) override;
};

class GrandparentAccess : public ASTNode
{
	unsigned int depth;
	std::string prop;

public:
	GrandparentAccess(unsigned int d,std::string p, int linenum = 0)
		:depth(d)
		,prop(p)
{
		my_line = linenum;

	}
	virtual const std::string class_name() const override { return "GrandparentAccess"; }
	virtual std::string dump(int indent) override
	{
		return std::string(indent, ' ') + "GrandparentAccess, property: " + prop + "; depth: " + std::to_string(depth) + "\n";
	}
	virtual Value resolve(Interpreter&) override;
	virtual Value& handle(Interpreter&) override;
};

class GlobalAccess : public ASTNode
{
public:
	std::string var;

	GlobalAccess(std::string v, int linenum = 0)
		:var(v)
{
		my_line = linenum;

	}
	virtual const std::string class_name() const override { return "GlobalAccess"; }
	virtual std::string dump(int indent) override
	{
		return std::string(indent, ' ') + "GlobalAccess, property: " + var + "\n";
	}
	virtual Value resolve(Interpreter&) override;
	virtual Value& handle(Interpreter&) override;
};

// a[i] and all that
class IndexAccess : public ASTNode
{
public:
	ASTNode* container;
	ASTNode* index;

	IndexAccess(ASTNode* c, ASTNode* i, int linenum = 0)
		:container(c)
		,index(i)
{
		my_line = linenum;

	}

	virtual Value resolve(Interpreter&) override;
	virtual Value& handle(Interpreter&) override;

	virtual const std::string class_name() const override { return "IndexAccess"; }
	virtual std::string dump(int indent) override
	{
		std::string ind = std::string(indent,' ');
		
		std::string str = ind + "IndexAccess:\n";

		str += ind + "*Container:\n";
		str += container->dump(indent + 1);
		str += ind + "[Index:\n";
		str += index->dump(indent + 1);
		str += ind + "]\n";

		return str;
	}
};

/*
Runs the associated code within the Try and tosses it at Catch if an exception occurs.
*/
class TryBlock : public Block
{
	std::string err_name;
	std::vector<Expression*> catch_statements; // TODO: Allow for multiple catchers based on error type :)
public:
	TryBlock(const std::vector<Expression*>& t, const std::string& err, const std::vector<Expression*>& c)
		:Block(t)
		,err_name(err)
		,catch_statements(c)
	{

	}


	virtual Value resolve(Interpreter&) override;
	virtual const std::string class_name() const override { return "TryBlock"; }

	virtual std::string dump(int indent)
	{
		const std::string ind = std::string(indent, ' ');
		std::string str = ind + "TryBlock\n";
		for (size_t i = 0; i < statements.size(); ++i)
		{
			str += statements[i]->dump(indent + 1);
		}
		str += ind + "?Catch: " + err_name + "\n";
		for (size_t i = 0; i < catch_statements.size(); ++i)
		{
			str += catch_statements[i]->dump(indent + 1);
		}
		return str;
	}
};

class ThrowStatement : public Expression
{
	ASTNode* err_node; // Node that is supposed to be an error object
public:
	ThrowStatement(ASTNode* e)
		:err_node(e)
	{

	}

	virtual Value resolve(Interpreter&) override;
	virtual const std::string class_name() const override { return "Throw"; }

	virtual std::string dump(int indent)
	{
		const std::string ind = std::string(indent, ' ');
		return ind + "Throw\n" + err_node->dump(indent + 1);
	}
};