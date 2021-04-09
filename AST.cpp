#include <iostream>

#include "AST.h"
#include "Interpreter.h"

#define BIN_ENUMS(a,b,c) ( (uint32_t(a) << 16) | (uint32_t(b) << 8)  | uint32_t(c) )

/*
* I've yet to be convinced that it's necessary for me to do this.
Value::~Value()
{
	if (t_vType == vType::String)
	{
		(*t_value.as_string_ptr).~std::string;
	}
	else if (t_vType == vType::Object)
	{
		delete(*(t_value.as_object_ptr));
	}
}
*/

Value ASTNode::resolve(Interpreter& interp)
{
	interp.RuntimeError(*this, "Attempted to resolve() an abstract ASTNode! (Type:" + class_name()  + ")");
	return Value();
}


//resolve()
Value Literal::resolve(Interpreter& interp)
{
	return heldval;
}
Value BinaryExpression::resolve(Interpreter& interp)
{
	//The Chef's ingredients: t_op, t_lhs, t_rhs
	Value lhs = t_lhs->resolve(interp);
	Value rhs = t_rhs->resolve(interp); //TODO: This fails the principle of short-circuiting but we'll be fine for now

	Value::vType lhs_type = lhs.t_vType;
	Value::vType rhs_type = rhs.t_vType;

	/*
	So for this we're going to do something a little wacky.
	I want this to be only one switch statement, but there's three dimensions of combinatorics:
	the operator used, the type of lhs, and the type of rhs.
	To do this, I am going to synthesize these three UInt8 enums into one Uint32 that is then switched against to find what to do.
	*/

	uint32_t switcher = BIN_ENUMS(t_op, lhs_type, rhs_type);

	switch (switcher)
	{
	//DOUBLE & DOUBLE
	case(BIN_ENUMS(bOps::Add, Value::vType::Double, Value::vType::Double)):
		return Value(lhs.t_value.as_double + rhs.t_value.as_double);
	case(BIN_ENUMS(bOps::Subtract, Value::vType::Double, Value::vType::Double)):
		return Value(lhs.t_value.as_double - rhs.t_value.as_double);
	case(BIN_ENUMS(bOps::Multiply, Value::vType::Double, Value::vType::Double)):
		return Value(lhs.t_value.as_double * rhs.t_value.as_double);
	case(BIN_ENUMS(bOps::Divide, Value::vType::Double, Value::vType::Double)):
		return Value(lhs.t_value.as_double / rhs.t_value.as_double);

	//DOUBLE & INT
	case(BIN_ENUMS(bOps::Add, Value::vType::Double, Value::vType::Integer)):
		return Value(lhs.t_value.as_double + rhs.t_value.as_int);
	case(BIN_ENUMS(bOps::Subtract, Value::vType::Double, Value::vType::Integer)):
		return Value(lhs.t_value.as_double - rhs.t_value.as_int);
	case(BIN_ENUMS(bOps::Multiply, Value::vType::Double, Value::vType::Integer)):
		return Value(lhs.t_value.as_double * rhs.t_value.as_int);
	case(BIN_ENUMS(bOps::Divide, Value::vType::Double, Value::vType::Integer)):
		return Value(lhs.t_value.as_double / rhs.t_value.as_int);

	//INT & DOUBLE
	case(BIN_ENUMS(bOps::Add, Value::vType::Integer, Value::vType::Double)):
		return Value(lhs.t_value.as_int + rhs.t_value.as_double);
	case(BIN_ENUMS(bOps::Subtract, Value::vType::Integer, Value::vType::Double)):
		return Value(lhs.t_value.as_int - rhs.t_value.as_double);
	case(BIN_ENUMS(bOps::Multiply, Value::vType::Integer, Value::vType::Double)):
		return Value(lhs.t_value.as_int * rhs.t_value.as_double);
	case(BIN_ENUMS(bOps::Divide, Value::vType::Integer, Value::vType::Double)):
		return Value(lhs.t_value.as_int / rhs.t_value.as_double);

	//INT & INT
	case(BIN_ENUMS(bOps::Add, Value::vType::Integer, Value::vType::Integer)):
		return Value(lhs.t_value.as_int + rhs.t_value.as_int);
	case(BIN_ENUMS(bOps::Subtract, Value::vType::Integer, Value::vType::Integer)):
		return Value(lhs.t_value.as_int - rhs.t_value.as_int);
	case(BIN_ENUMS(bOps::Multiply, Value::vType::Integer, Value::vType::Integer)):
		return Value(lhs.t_value.as_int * rhs.t_value.as_int);
	case(BIN_ENUMS(bOps::Divide, Value::vType::Integer, Value::vType::Integer)):
		return Value(lhs.t_value.as_int / rhs.t_value.as_int);

	default:
		interp.RuntimeError(*this, "Failed to do a binary operation!");
		return Value();
	}
}


Value Function::resolve(Interpreter & interp)
{
	for (auto it = statements.begin(); it != statements.end(); ++it)
	{
		//it is a pointer to a pointer to an Expression.
		Expression* ptr = *it;
		if (ptr->class_name() == "ReturnStatement")
		{
			ReturnStatement rt = *((ReturnStatement*)ptr); // Is this safe?

			if (rt.has_expr) // If this actually has something to return
				return rt.resolve(interp); // Do that
			break;// otherwise use the default returnValue
		}
		else
		{
			ptr->resolve(interp); // I dunno.
		}
	}
	return returnValue;
}

Value CallExpression::resolve(Interpreter& interp)
{
	return interp.get_func(func_name)->resolve(interp);
}