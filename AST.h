#pragma once

//#define NDEBUG
#include <assert.h> 

#include "Forward.h"

class Value { // A general pseudo-typeless Value used to store data within the programming language.

public:
	enum class vType : uint8_t {
		Null,
		Bool,
		Integer,
		Double,
		String,
		Object
	}t_vType{ vType::Null };

	union {
		bool as_bool;
		int as_int;
		double as_double;
		std::string* as_string_ptr;
		Object* as_object_ptr;
	}t_value;

	//Constructors
	Value()
	{

	}
	~Value();
	Value(int i)
	{
		t_value.as_int = i;
		t_vType = vType::Integer;
	}
	Value(double d)
	{
		t_value.as_double = d;
		t_vType = vType::Double;
	}
	Value(bool b)
	{
		t_value.as_bool = b;
		t_vType = vType::Bool;
	}
	Value(std::string s)
	{
		std::string* our_str = new std::string(s);
		t_value.as_string_ptr = our_str;
		t_vType = vType::String;
		//std::cout << "Construction successful: " + *(t_value.as_string_ptr);
	}
	Value(Object& o)
	{
		t_value.as_object_ptr = &o;
		t_vType = vType::Object;
	}

	std::string to_string();
	std::string typestring();
};

class ASTNode // ASTNodes are abstract symbols which together form a "flow chart" tree of symbols that the parser creates from the text that the interpreter then interprets.
{

public:
	virtual const std::string class_name() const { return "ASTNode"; }
	virtual Value resolve(Interpreter&); // Collapses this symbol into a real dang thing or process that the interpreter can do.
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
};


class Expression : public ASTNode
{

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
	virtual Value resolve(Interpreter&) override;
	std::string get_str()
	{
		return t_name;
	}
	virtual const std::string class_name() const override { return "Identifier"; }
};

class AssignmentStatement : public Expression
{
	enum class aOps : uint8_t {
		Assign, // =
		AssignAdd, // +=
		AssignSubtract, // -=
		AssignMultiply, // *=
		AssignDivide // /=
	}t_op;
	Identifier* id;
	ASTNode* rhs;
public:
	AssignmentStatement(Identifier* i, ASTNode* r, aOps o = aOps::Assign)
		:id(i),
		rhs(r),
		t_op(o)
	{
		//std::cout << "My identifier has the name " + id->get_str() + "!\n";
	}
	virtual Value resolve(Interpreter&) override;
};

class UnaryExpression : public Expression
{
	enum class uOps : uint8_t {
		Not,
		Negate,
		BitwiseNot
	}t_op;
	ASTNode* t_rhs;
	UnaryExpression(uOps Operator, ASTNode* r)
		:t_op(Operator)
		,t_rhs(r)
	{

	}
	virtual Value resolve(Interpreter&) override;
};

class BinaryExpression : public Expression 
{ // An ASTNode which is an operation between two, smaller Expressions.

public:
	enum class bOps : uint8_t {
		Add,
		Subtract,
		Multiply,
		Divide
	}t_op;
	ASTNode* t_lhs, *t_rhs;

	BinaryExpression(bOps Operator, ASTNode* l, ASTNode* r)
		:t_lhs(l) // What the fuck is this syntax holy shit
		,t_rhs(r)
		,t_op(Operator)
	{
		
	}
	virtual Value resolve(Interpreter&) override;
};

class ReturnStatement : public Expression {
	ASTNode *held_expr;
public:
	bool has_expr{ false };

	virtual const std::string class_name() const override { return "ReturnStatement"; }

	ReturnStatement()
		:held_expr(&Literal(Value()))
	{

	}
	ReturnStatement(ASTNode* node)
		:held_expr(node)
	{
		has_expr = true;
	}
	virtual Value resolve(Interpreter& interp) override
	{
		return held_expr->resolve(interp);
	}
};

class CallExpression : public Expression {
	std::string func_name;
public:
	CallExpression(std::string str)
		:func_name(str)
	{

	}
	CallExpression(Value &v)
	{
		assert(v.t_vType == Value::vType::String);
		func_name = *(v.t_value.as_string_ptr);
	}

	virtual Value resolve(Interpreter&) override;
};

class Function : public ASTNode
{
	Value returnValue = Value(); // My return value
	std::vector<Expression*> statements; // The statements which're executed when I am run
	//std::vector<Expression> args;
	std::string t_name; // My name
public:
	Function(std::string name, Expression* expr)
	{
		statements = std::vector<Expression*>{ expr };
	}
	Function(std::string name, std::vector<Expression*> exprs) // Hopefully this works. :(
	{
		t_name = name;
		statements = exprs;
	}
	Function* append(Expression* expr)
	{
		statements.push_back(expr);
		return this;
	}

	virtual Value resolve(Interpreter&) override;
	virtual const std::string class_name() const override { return "Function"; }
};