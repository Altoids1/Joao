#include "Forward.h"
#include "AST.h"
#include "Object.h"
#include "Interpreter.h"

#define BIN_ENUMS(a,b,c) ( (uint32_t(a) << 16) | (uint32_t(b) << 8)  | uint32_t(c) )
#define UN_ENUMS(a,b) ((uint32_t(a) << 8)  | uint32_t(b) )

//It's necessary for me to do this.
Value::~Value()
{
	//std::cout << "*Yoda Scream*\n";
	//TODO: Make sure we're not the only being in the universe that has a pointer to these things.
	/*
	if (t_vType == vType::String)
	{
		delete t_value.as_string_ptr;
	}
	else if (t_vType == vType::Object)
	{
		delete t_value.as_object_ptr;
	}
	*/
}


std::string Value::to_string()
{
	switch (t_vType)
	{
	case(vType::Null):
		return "NULL";
	case(vType::Bool):
		return std::to_string(t_value.as_bool);
	case(vType::Integer):
		return std::to_string(t_value.as_int);
	case(vType::Double):
		return std::to_string(t_value.as_double);
	case(vType::String):
		return *(t_value.as_string_ptr);
	case(vType::Object):
		return t_value.as_object_ptr->dump();
	default:
		return "???";
	}
}

std::string Value::typestring()
{
	switch (t_vType)
	{
	case(vType::Null):
		return "NULL";
	case(vType::Bool):
		return "Boolean";
	case(vType::Integer):
		return "Integer";
	case(vType::Double):
		return "Double";
	case(vType::String):
		return "String";
	case(vType::Object):
		return "Object";
	default:
		return "UNKNOWN!!";
	}
}

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

Value Identifier::resolve(Interpreter& interp)
{
	// So to "resolve" this means to get its current value, if it has one.
	//Some uses of this class may actually not call the resolve function, and instead simply use it to store the string name of a variable,
	//so as to be able to set it within a scope or, whatever, like for assignment and assignment-y purposes.

	return interp.get_var(t_name,this);
}

Value AssignmentStatement::resolve(Interpreter& interp)
{
	Value rhs_val = rhs->resolve(interp);
	//std::cout << "Their name is " + id->get_str() + " and their value is " + std::to_string(rhs_val.t_value.as_int) + "\n";

	if (t_op != aOps::Assign)
	{
		interp.RuntimeError(*this, "Attempt to call unimplemented Assignment operation: " + (int)t_op);
	}

	interp.override_var(id->get_str(), rhs_val, this);

	return rhs_val;
}

Value LocalAssignmentStatement::resolve(Interpreter& interp)
{
	Value rhs_val = rhs->resolve(interp);
	//std::cout << "Their name is " + id->get_str() + " and their value is " + std::to_string(rhs_val.t_value.as_int) + "\n";

	if (t_op != aOps::Assign)
	{
		interp.RuntimeError(*this, "Attempt to call unimplemented Assignment operation: " + (int)t_op);
	}

	interp.set_var(id->get_str(), rhs_val, this);

	return rhs_val;
}

Value UnaryExpression::resolve(Interpreter& interp)
{
	Value rhs = t_rhs->resolve(interp);
	Value::vType rtype = rhs.t_vType;


	uint32_t switcher = UN_ENUMS(t_op, rtype);

	switch (switcher)
	{
	//NEGATE
	case(UN_ENUMS(uOps::Negate,Value::vType::Integer)):
		return Value(-rhs.t_value.as_int);
	case(UN_ENUMS(uOps::Negate, Value::vType::Double)):
		return Value(-rhs.t_value.as_double);
	//NOT
	case(UN_ENUMS(uOps::Not, Value::vType::Null)): // !NULL === NULL
		return Value();
	case(UN_ENUMS(uOps::Not, Value::vType::Integer)):
		return Value(!rhs.t_value.as_int);
	case(UN_ENUMS(uOps::Not, Value::vType::Double)):
		return Value(!rhs.t_value.as_double);
	//BITWISENOT
	case(UN_ENUMS(uOps::BitwiseNot, Value::vType::Integer)):
		return Value(~rhs.t_value.as_int);
	case(UN_ENUMS(uOps::BitwiseNot, Value::vType::Double)): // Does, indeed, flip the bits of the double
	{
		uint64_t fauxint = ~*(reinterpret_cast<uint64_t*>(&rhs.t_value.as_double));
		double newdouble = *(reinterpret_cast<double*>(&fauxint));
		return Value(newdouble);
	}

	default:
		interp.RuntimeError(*this, "Failed to do an Unary operation! (" + rhs.to_string() + ")\nType: (" + rhs.typestring() + ")");
		return Value();
	}
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
	//
	case(BIN_ENUMS(bOps::FloorDivide, Value::vType::Double, Value::vType::Double)):
		return Value(floor(lhs.t_value.as_double / rhs.t_value.as_double));
	case(BIN_ENUMS(bOps::Exponent, Value::vType::Double, Value::vType::Double)):
		return Value(pow(lhs.t_value.as_double, rhs.t_value.as_double));
	case(BIN_ENUMS(bOps::Modulo, Value::vType::Double, Value::vType::Double)):
		return Value(modf(lhs.t_value.as_double/rhs.t_value.as_double, nullptr));
	//
		//Bitwise :(
	//
	case(BIN_ENUMS(bOps::Concatenate, Value::vType::Double, Value::vType::Double)):
		return Value(std::to_string(lhs.t_value.as_double) + std::to_string(rhs.t_value.as_double));
	//
	case(BIN_ENUMS(bOps::LessThan, Value::vType::Double, Value::vType::Double)):
		return Value(lhs.t_value.as_double < rhs.t_value.as_double);
	case(BIN_ENUMS(bOps::LessEquals, Value::vType::Double, Value::vType::Double)):
		return Value(lhs.t_value.as_double <= rhs.t_value.as_double);
	case(BIN_ENUMS(bOps::Greater, Value::vType::Double, Value::vType::Double)):
		return Value(lhs.t_value.as_double > rhs.t_value.as_double);
	case(BIN_ENUMS(bOps::GreaterEquals, Value::vType::Double, Value::vType::Double)):
		return Value(lhs.t_value.as_double >= rhs.t_value.as_double);
	case(BIN_ENUMS(bOps::Equals, Value::vType::Double, Value::vType::Double)):
		return Value(lhs.t_value.as_double == rhs.t_value.as_double);
	case(BIN_ENUMS(bOps::NotEquals, Value::vType::Double, Value::vType::Double)):
		return Value(lhs.t_value.as_double != rhs.t_value.as_double);
	case(BIN_ENUMS(bOps::LogicalAnd, Value::vType::Double, Value::vType::Double)):
		return Value(lhs.t_value.as_double && rhs.t_value.as_double);
	case(BIN_ENUMS(bOps::LogicalOr, Value::vType::Double, Value::vType::Double)):
		return Value(lhs.t_value.as_double || rhs.t_value.as_double);
	case(BIN_ENUMS(bOps::LogicalXor, Value::vType::Double, Value::vType::Double)):
		return Value((lhs.t_value.as_double || rhs.t_value.as_double) && !(lhs.t_value.as_double && rhs.t_value.as_double));


	//DOUBLE & INT
	case(BIN_ENUMS(bOps::Add, Value::vType::Double, Value::vType::Integer)):
		return Value(lhs.t_value.as_double + rhs.t_value.as_int);
	case(BIN_ENUMS(bOps::Subtract, Value::vType::Double, Value::vType::Integer)):
		return Value(lhs.t_value.as_double - rhs.t_value.as_int);
	case(BIN_ENUMS(bOps::Multiply, Value::vType::Double, Value::vType::Integer)):
		return Value(lhs.t_value.as_double * rhs.t_value.as_int);
	case(BIN_ENUMS(bOps::Divide, Value::vType::Double, Value::vType::Integer)):
		return Value(lhs.t_value.as_double / rhs.t_value.as_int);
	//
	case(BIN_ENUMS(bOps::FloorDivide, Value::vType::Double, Value::vType::Integer)):
		return Value(floor(lhs.t_value.as_double / rhs.t_value.as_int));
	case(BIN_ENUMS(bOps::Exponent, Value::vType::Double, Value::vType::Integer)):
		return Value(pow(lhs.t_value.as_double, rhs.t_value.as_int));
	case(BIN_ENUMS(bOps::Modulo, Value::vType::Double, Value::vType::Integer)):
		return Value(modf(lhs.t_value.as_double / rhs.t_value.as_int, nullptr));
	//
		//Bitwise :(
	//
	case(BIN_ENUMS(bOps::Concatenate, Value::vType::Double, Value::vType::Integer)):
		return Value(std::to_string(lhs.t_value.as_double) + std::to_string(rhs.t_value.as_int));
	//
	case(BIN_ENUMS(bOps::LessThan, Value::vType::Double, Value::vType::Integer)):
		return Value(lhs.t_value.as_double < rhs.t_value.as_int);
	case(BIN_ENUMS(bOps::LessEquals, Value::vType::Double, Value::vType::Integer)):
		return Value(lhs.t_value.as_double <= rhs.t_value.as_int);
	case(BIN_ENUMS(bOps::Greater, Value::vType::Double, Value::vType::Integer)):
		return Value(lhs.t_value.as_double > rhs.t_value.as_int);
	case(BIN_ENUMS(bOps::GreaterEquals, Value::vType::Double, Value::vType::Integer)):
		return Value(lhs.t_value.as_double >= rhs.t_value.as_int);
	case(BIN_ENUMS(bOps::Equals, Value::vType::Double, Value::vType::Integer)):
		return Value(lhs.t_value.as_double == rhs.t_value.as_int);
	case(BIN_ENUMS(bOps::NotEquals, Value::vType::Double, Value::vType::Integer)):
		return Value(lhs.t_value.as_double != rhs.t_value.as_int);
	case(BIN_ENUMS(bOps::LogicalAnd, Value::vType::Double, Value::vType::Integer)):
		return Value(lhs.t_value.as_double && rhs.t_value.as_int);
	case(BIN_ENUMS(bOps::LogicalOr, Value::vType::Double, Value::vType::Integer)):
		return Value(lhs.t_value.as_double || rhs.t_value.as_int);
	case(BIN_ENUMS(bOps::LogicalXor, Value::vType::Double, Value::vType::Integer)):
		return Value((lhs.t_value.as_double || rhs.t_value.as_int) && !(lhs.t_value.as_double && rhs.t_value.as_int));



	//INT & DOUBLE
	case(BIN_ENUMS(bOps::Add, Value::vType::Integer, Value::vType::Double)):
		return Value(lhs.t_value.as_int + rhs.t_value.as_double);
	case(BIN_ENUMS(bOps::Subtract, Value::vType::Integer, Value::vType::Double)):
		return Value(lhs.t_value.as_int - rhs.t_value.as_double);
	case(BIN_ENUMS(bOps::Multiply, Value::vType::Integer, Value::vType::Double)):
		return Value(lhs.t_value.as_int * rhs.t_value.as_double);
	case(BIN_ENUMS(bOps::Divide, Value::vType::Integer, Value::vType::Double)):
		return Value(lhs.t_value.as_int / rhs.t_value.as_double);
	//
	case(BIN_ENUMS(bOps::FloorDivide, Value::vType::Integer, Value::vType::Double)):
		return Value(floor(lhs.t_value.as_int / rhs.t_value.as_double));
	case(BIN_ENUMS(bOps::Exponent, Value::vType::Integer, Value::vType::Double)):
		return Value(pow(lhs.t_value.as_int, rhs.t_value.as_double));
	case(BIN_ENUMS(bOps::Modulo, Value::vType::Integer, Value::vType::Double)):
		return Value(modf(lhs.t_value.as_int / rhs.t_value.as_double, nullptr));
	//
		//TODO: Bitwise stuff with doubles, too
	//
	case(BIN_ENUMS(bOps::Concatenate, Value::vType::Integer, Value::vType::Double)):
		return Value(std::to_string(lhs.t_value.as_int) + std::to_string(rhs.t_value.as_double));
	//
	case(BIN_ENUMS(bOps::LessThan, Value::vType::Integer, Value::vType::Double)):
		return Value(lhs.t_value.as_int < rhs.t_value.as_double);
	case(BIN_ENUMS(bOps::LessEquals, Value::vType::Integer, Value::vType::Double)):
		return Value(lhs.t_value.as_int <= rhs.t_value.as_double);
	case(BIN_ENUMS(bOps::Greater, Value::vType::Integer, Value::vType::Double)):
		return Value(lhs.t_value.as_int > rhs.t_value.as_double);
	case(BIN_ENUMS(bOps::GreaterEquals, Value::vType::Integer, Value::vType::Double)):
		return Value(lhs.t_value.as_int >= rhs.t_value.as_double);
	case(BIN_ENUMS(bOps::Equals, Value::vType::Integer, Value::vType::Double)):
		return Value(lhs.t_value.as_int == rhs.t_value.as_double);
	case(BIN_ENUMS(bOps::NotEquals, Value::vType::Integer, Value::vType::Double)):
		return Value(lhs.t_value.as_int != rhs.t_value.as_double);
	case(BIN_ENUMS(bOps::LogicalAnd, Value::vType::Integer, Value::vType::Double)):
		return Value(lhs.t_value.as_int && rhs.t_value.as_double);
	case(BIN_ENUMS(bOps::LogicalOr, Value::vType::Integer, Value::vType::Double)):
		return Value(lhs.t_value.as_int || rhs.t_value.as_double);
	case(BIN_ENUMS(bOps::LogicalXor, Value::vType::Integer, Value::vType::Double)):
		return Value((lhs.t_value.as_int || rhs.t_value.as_double) && !(lhs.t_value.as_int && rhs.t_value.as_double));


	//INT & INT
	case(BIN_ENUMS(bOps::Add, Value::vType::Integer, Value::vType::Integer)):
		return Value(lhs.t_value.as_int + rhs.t_value.as_int);
	case(BIN_ENUMS(bOps::Subtract, Value::vType::Integer, Value::vType::Integer)):
		return Value(lhs.t_value.as_int - rhs.t_value.as_int);
	case(BIN_ENUMS(bOps::Multiply, Value::vType::Integer, Value::vType::Integer)):
		return Value(lhs.t_value.as_int * rhs.t_value.as_int);
	case(BIN_ENUMS(bOps::Divide, Value::vType::Integer, Value::vType::Integer)):
	//
	case(BIN_ENUMS(bOps::FloorDivide, Value::vType::Integer, Value::vType::Integer)): // :)
		return Value(lhs.t_value.as_int / rhs.t_value.as_int);
	case(BIN_ENUMS(bOps::Exponent, Value::vType::Integer, Value::vType::Integer)):
		return Value(static_cast<int>(pow(lhs.t_value.as_int, rhs.t_value.as_int)));
	case(BIN_ENUMS(bOps::Modulo, Value::vType::Integer, Value::vType::Integer)):
		return Value(lhs.t_value.as_int % rhs.t_value.as_int);
	//
	case(BIN_ENUMS(bOps::BitwiseAnd, Value::vType::Integer, Value::vType::Integer)):
		return Value(lhs.t_value.as_int & rhs.t_value.as_int);
	case(BIN_ENUMS(bOps::BitwiseXor, Value::vType::Integer, Value::vType::Integer)):
		return Value(lhs.t_value.as_int ^ rhs.t_value.as_int);
	case(BIN_ENUMS(bOps::BitwiseOr, Value::vType::Integer, Value::vType::Integer)):
		return Value(lhs.t_value.as_int | rhs.t_value.as_int);
	case(BIN_ENUMS(bOps::ShiftRight, Value::vType::Integer, Value::vType::Integer)):
		return Value(lhs.t_value.as_int >> rhs.t_value.as_int);
	case(BIN_ENUMS(bOps::ShiftLeft, Value::vType::Integer, Value::vType::Integer)):
		return Value(lhs.t_value.as_int << rhs.t_value.as_int);
	//
	case(BIN_ENUMS(bOps::Concatenate, Value::vType::Integer, Value::vType::Integer)):
		return Value(std::to_string(lhs.t_value.as_int) + std::to_string(rhs.t_value.as_int));
	//
	case(BIN_ENUMS(bOps::LessThan, Value::vType::Integer, Value::vType::Integer)):
		return Value(lhs.t_value.as_int < rhs.t_value.as_int);
	case(BIN_ENUMS(bOps::LessEquals, Value::vType::Integer, Value::vType::Integer)):
		return Value(lhs.t_value.as_int <= rhs.t_value.as_int);
	case(BIN_ENUMS(bOps::Greater, Value::vType::Integer, Value::vType::Integer)):
		return Value(lhs.t_value.as_int > rhs.t_value.as_int);
	case(BIN_ENUMS(bOps::GreaterEquals, Value::vType::Integer, Value::vType::Integer)):
		return Value(lhs.t_value.as_int >= rhs.t_value.as_int);
	case(BIN_ENUMS(bOps::Equals, Value::vType::Integer, Value::vType::Integer)):
		return Value(lhs.t_value.as_int == rhs.t_value.as_int);
	case(BIN_ENUMS(bOps::NotEquals, Value::vType::Integer, Value::vType::Integer)):
		return Value(lhs.t_value.as_int != rhs.t_value.as_int);
	case(BIN_ENUMS(bOps::LogicalAnd, Value::vType::Integer, Value::vType::Integer)):
		return Value(lhs.t_value.as_int && rhs.t_value.as_int);
	case(BIN_ENUMS(bOps::LogicalOr, Value::vType::Integer, Value::vType::Integer)):
		return Value(lhs.t_value.as_int || rhs.t_value.as_int);
	case(BIN_ENUMS(bOps::LogicalXor, Value::vType::Integer, Value::vType::Integer)):
		return Value((lhs.t_value.as_int || rhs.t_value.as_int) && !(lhs.t_value.as_int && rhs.t_value.as_int));



	//STRING & STRING
	case(BIN_ENUMS(bOps::Concatenate, Value::vType::String, Value::vType::String)):
	{
		std::string newstr = *(lhs.t_value.as_string_ptr) + *(rhs.t_value.as_string_ptr);
		return Value(newstr);
	}
	//STRING & INT
	case(BIN_ENUMS(bOps::Concatenate, Value::vType::String, Value::vType::Integer)):
	{
		std::string newstr = *(lhs.t_value.as_string_ptr) + std::to_string(rhs.t_value.as_int);
		return Value(newstr);
	}
	//INT & STRING
	case(BIN_ENUMS(bOps::Concatenate, Value::vType::Integer, Value::vType::String)):
	{
		std::string newstr = std::to_string(lhs.t_value.as_int) + *(rhs.t_value.as_string_ptr);
		return Value(newstr);
	}
	//STRING & DOUBLE
	case(BIN_ENUMS(bOps::Concatenate, Value::vType::String, Value::vType::Double)):
	{
		std::string newstr = *(lhs.t_value.as_string_ptr) + std::to_string(rhs.t_value.as_double);
		return Value(newstr);
	}
	//DOUBLE & STRING
	case(BIN_ENUMS(bOps::Concatenate, Value::vType::Double, Value::vType::String)):
	{
		std::string newstr = std::to_string(lhs.t_value.as_double) + *(rhs.t_value.as_string_ptr);
		return Value(newstr);
	}

	default:
		interp.RuntimeError(*this, "Failed to do a binary operation! (" + lhs.to_string() + ", " + rhs.to_string() + ")\nTypes: (" + lhs.typestring() + ", " + rhs.typestring() + ")");
		return Value();
	}
}


void Function::give_args(std::vector<Value>& args, Interpreter& interp)
{
	t_args = args;
	if (t_args.size() < t_argnames.size()) // If we were not given enough arguments
	{
		interp.RuntimeError(*this, "Insufficient amonut of arguments given!");
	}
}
Value Function::resolve(Interpreter & interp)
{
	interp.push_stack(Directory::lastword(t_name));
	for (auto it = statements.begin(); it != statements.end(); ++it)
	{
		//it is a pointer to a pointer to an Expression.
		Expression* ptr = *it;
		if (ptr->class_name() == "ReturnStatement")
		{
			ReturnStatement rt = *(static_cast<ReturnStatement*>(ptr));

			if (rt.has_expr) // If this actually has something to return
			{
				Value ret = rt.resolve(interp); // Resolve it first
				interp.pop_stack(); // THEN pop the stack
				return ret; // THEN return it
			}
			break;// otherwise use the default returnValue
		}
		else
		{
			Value vuh = ptr->resolve(interp); // I dunno.
			if (interp.FORCE_RETURN)
			{
				interp.FORCE_RETURN = false;
				interp.pop_stack();
				return vuh;
			}
		}
	}
	interp.pop_stack();
	return returnValue;
}

Value CallExpression::resolve(Interpreter& interp)
{
	std::vector<Value> vargs;
	for (auto it = args.begin(); it != args.end(); ++it)
	{
		Expression* e = *it;
		vargs.push_back(e->resolve(interp));
	}
	Function* ourfunc = interp.get_func(func_name, this);
	ourfunc->give_args(vargs,interp);
	return ourfunc->resolve(interp);
}

Value NativeFunction::resolve(Interpreter& interp)
{
	Value result = lambda(t_args);
	if (result.t_vType == Value::vType::Null && result.t_value.as_int)
	{
		switch (result.t_value.as_int)
		{
		case(int(Program::ErrorCode::BadArgType)):
			interp.RuntimeError(*this, "Args of improper type given to NativeFunction!");
			break;
		case(int(Program::ErrorCode::NotEnoughArgs)):
			interp.RuntimeError(*this, "Not enough args provided to NativeFunction!");
			break;
		default:
			interp.RuntimeError(*this, "Unknown RuntimeError in NativeFunction!");
		}
	}
	return result; // Woag.
}

Value Block::iterate_statements(Interpreter& interp)
{
	for (auto it = statements.begin(); it != statements.end(); ++it)
	{
		//it is a pointer to a pointer to an Expression.
		Expression* ptr = *it;
		if (ptr->class_name() == "ReturnStatement")
		{
			ReturnStatement rt = *((ReturnStatement*)ptr); // Is this safe?

			interp.FORCE_RETURN = true;
			if (rt.has_expr) // If this actually has something to return
			{
				Value ret = rt.resolve(interp); // Get the return		
				interp.pop_stack(); // THEN pop the stack
				return ret; // THEN return the value.
			}
			interp.pop_stack();
			return Value();
		}
		else
		{
			Value vuh = ptr->resolve(interp); // I dunno.
			if (interp.FORCE_RETURN)
			{
				interp.pop_stack();
				return vuh;
			}
		}
	}
	return Value();
}

Value IfBlock::resolve(Interpreter& interp)
{
	if (!condition)
		return Value();

	if (!condition->resolve(interp)) // If our condition exists (so we're not an Else) and fails
	{
		if (Elseif) // If we have an Elseif to point to
			return Elseif->resolve(interp); // Return that
		return Value(); // Otherwise return Null, I guess.
	}

	if (condition->resolve(interp))
	{
		std::cout << "Wuh?\n";
	}

	interp.push_stack("if");
	Value blockret = iterate_statements(interp);
	
	if (interp.FORCE_RETURN)
		return blockret;
	else
		interp.pop_stack();
	return Value();
}

Value ForBlock::resolve(Interpreter& interp)
{
	interp.push_stack("for"); // Has to happen here since initializer takes place in the for-loops var stack
	if (initializer)
		initializer->resolve(interp);
	while (condition && condition->resolve(interp))
	{
		Value blockret = iterate_statements(interp);
		if (interp.FORCE_RETURN)
			return blockret;
		increment->resolve(interp);
	}
	interp.pop_stack();
	return Value();
}

Value WhileBlock::resolve(Interpreter& interp)
{
	interp.push_stack("while");
	if(!condition) // if condition not defined
		interp.RuntimeError(*this, "Missing condition in WhileBlock!");
	/*
	if (!(condition->resolve(interp)))
	{
		std::cout << "THIS! SENTENCE! IS! FALSE!dontthinkaboutitdontthinkaboutitdontthinkaboutitdontthinkaboutitdontthinkaboutit...\n"; // Have to inform the computer to not think about the paradox
	}
	*/
	while (condition->resolve(interp))
	{
		Value blockret = iterate_statements(interp);
		if (interp.FORCE_RETURN)
		{
			//std::cout << "Whileloop got to FORCE_RETURN!\n";
			return blockret;
		}
	}
	interp.pop_stack();
	return Value();
}