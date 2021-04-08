#pragma once
#include <string>
#include <unordered_map>
#include <vector>

//#define NDEBUG
#include <assert.h> 

#include "Forward.h"
#include "Object.h"

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
	Value();
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
	Value(std::string& s)
	{
		t_value.as_string_ptr = &s;
		t_vType = vType::String;
	}
	Value(Object& o)
	{
		t_value.as_object_ptr = &o;
		t_vType = vType::Object;
	}
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
	Literal(Value);
	virtual Value resolve(Interpreter&) override;
};


class Expression : public ASTNode
{

};

class BinaryExpression : public Expression 
{ // An ASTNode which is an operation between two, smaller Expressions.
	enum class bOps : uint8_t {
		Add,
		Subtract,
		Multiply,
		Divide
	}t_op;
	ASTNode t_lhs, t_rhs;

	BinaryExpression(bOps Operator, ASTNode &l, ASTNode &r)
		:t_lhs(l) // What the fuck is this syntax holy shit
		,t_rhs(r)
		,t_op(Operator)
	{
		
	}
	virtual Value resolve(Interpreter&) override;
};

class ReturnStatement : public Expression {
	ASTNode held_expr;
public:
	bool has_expr{ false };
	ReturnStatement()
		:held_expr(Literal(Value()))
	{

	}
	ReturnStatement(ASTNode& node)
		:held_expr(node)
	{
		has_expr = true;
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
	std::vector<Expression> statements; // The statements which're executed when I am run
	std::string t_name; // My name
public:
	Function(std::string name, std::vector<Expression>& exprs)
	{
		t_name = name;
		statements = exprs;
	}
	virtual Value resolve(Interpreter&) override;
};