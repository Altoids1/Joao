#include "../Program.h"
#include "../AST.hpp"

#include <limits>

#define MATH_E 2.71828182845904523536
#define PI 3.14159265358979323846

#define NATIVE_FUNC(name) definedFunctions[ name ] = static_cast<Function*>(new NativeFunction( name , [](std::vector<Value> args)

#define MAXMIN_ENUM(type,thebool) (static_cast<uint16_t>(type) | (static_cast<uint16_t>(thebool) << 8))


Value math::round(std::vector<Value> args)
{
	if(args.empty())
		return Value(Value::vType::Null, int(ErrorCode::NotEnoughArgs));

	switch (args[0].t_vType)
	{
	default:
		return Value(Value::vType::Null, int(ErrorCode::BadArgType));
	case(Value::vType::Double):
	{
		double dee = args[0].t_value.as_double;
		if (ceil(dee) - dee < 0.5)
			return Value(static_cast<int64_t>(ceil(dee)));
		else
			return Value(static_cast<int64_t>(floor(dee)));
	}
	case(Value::vType::Integer):
		return args[0];
	}

	return Value(); // Just in case
}


void Program::construct_math_library()
{
	//MATHEMATICS

	//GLOBAL VALUES
	globals.table["PI"] = new Value(PI);
	globals.table["EULER"] = new Value(MATH_E);

	//GLOBAL FUNCTIONS
	NATIVE_FUNC("abs")
	{
		Value arg = args[0];
		switch (arg.t_vType)
		{
		case(Value::vType::Double):
			return Value(abs(arg.t_value.as_double));
		case(Value::vType::Integer):
			return Value(abs(arg.t_value.as_int));
		case(Value::vType::Bool):
			return arg;
		default:
			return Value(Value::vType::Null, int(ErrorCode::BadArgType));
		}
	}));
	NATIVE_FUNC("acos")
	{
		Value arg = args[0];
		switch (arg.t_vType)
		{
		case(Value::vType::Double):
			return Value(acos(arg.t_value.as_double));
		case(Value::vType::Integer):
			return Value(acos(arg.t_value.as_int));
		case(Value::vType::Bool):
			return Value(acos(arg.t_value.as_bool));
		default:
			return Value(Value::vType::Null, int(ErrorCode::BadArgType));
		}
	}));
	NATIVE_FUNC("asin")
	{
		Value arg = args[0];
		switch (arg.t_vType)
		{
		case(Value::vType::Double):
			return Value(asin(arg.t_value.as_double));
		case(Value::vType::Integer):
			return Value(asin(arg.t_value.as_int));
		case(Value::vType::Bool):
			return Value(asin(arg.t_value.as_bool));
		default:
			return Value(Value::vType::Null, int(ErrorCode::BadArgType));
		}
	}));
	NATIVE_FUNC("atan")
	{
		Value arg = args[0];
		switch (arg.t_vType)
		{
		case(Value::vType::Double):
			return Value(atan(arg.t_value.as_double));
		case(Value::vType::Integer):
			return Value(atan(arg.t_value.as_int));
		case(Value::vType::Bool):
			return Value(atan(arg.t_value.as_bool));
		default:
			return Value(Value::vType::Null, int(ErrorCode::BadArgType));
		}
	}));
	NATIVE_FUNC("ceil")
	{
		Value arg = args[0];
		switch (arg.t_vType)
		{
		case(Value::vType::Double):
			return Value(ceil(arg.t_value.as_double));
		case(Value::vType::Integer):
			return arg;
		case(Value::vType::Bool):
			return arg;
		default:
			return Value(Value::vType::Null, int(ErrorCode::BadArgType));
		}
	}));

	NATIVE_FUNC("cos")
	{
		Value arg = args[0];
		switch (arg.t_vType)
		{
		case(Value::vType::Double):
			return Value(cos(arg.t_value.as_double));
		case(Value::vType::Integer):
			return Value(cos(arg.t_value.as_int));
		case(Value::vType::Bool):
			return Value(cos(arg.t_value.as_bool));
		default:
			return Value(Value::vType::Null, int(ErrorCode::BadArgType));
		}
	}));
	NATIVE_FUNC("deg")
	{
		Value arg = args[0];
		switch (arg.t_vType)
		{
		case(Value::vType::Double):
			return Value(arg.t_value.as_double * (180 / PI));
		case(Value::vType::Integer):
			return Value(arg.t_value.as_int * (180 / PI));
		case(Value::vType::Bool):
			return Value(arg.t_value.as_bool * (180 / PI));
		default:
			return Value(Value::vType::Null, int(ErrorCode::BadArgType));
		}
	}));
	NATIVE_FUNC("exp")
	{
		Value arg = args[0];

		//Literal eugh = Literal(Value(MATH_E));
		//Literal argh = Literal(arg);
		switch (arg.t_vType)
		{
		case(Value::vType::Double):
			//return BinaryExpression(BinaryExpression::bOps::Exponent, &eugh,&argh).resolve(); //FIXME: Make it do this instead
			return Value(exp(arg.t_value.as_double));
		case(Value::vType::Integer):
			return Value(exp(arg.t_value.as_int));
		case(Value::vType::Bool):
			return Value(exp(arg.t_value.as_bool));
		default:
			return Value(Value::vType::Null, int(ErrorCode::BadArgType));
		}
	}));
	NATIVE_FUNC("floor")
	{
		Value arg = args[0];
		switch (arg.t_vType)
		{
		case(Value::vType::Double):
			return Value(floor(arg.t_value.as_double));
		case(Value::vType::Integer):
			return arg;
		case(Value::vType::Bool):
			return arg;
		default:
			return Value(Value::vType::Null, int(ErrorCode::BadArgType));
		}
	}));
	NATIVE_FUNC("max")//FIXME: Use BinaryOperation(bOp::Greater) instead of C++'s greater-than operator, for overloading and not-writing-things-in-many-places.
	{
		if (args.size() == 0)
			return Value();

		Value biggest = args[0];
		union
		{
			double d;
			Value::JoaoInt i;
		}bigval;
		bool is_double;
		switch (biggest.t_vType)
		{
		case(Value::vType::Double):
			bigval.d = biggest.t_value.as_double;
			is_double = true;
		case(Value::vType::Integer):
			bigval.i = biggest.t_value.as_int;
			is_double = false;
		default:
			return Value(Value::vType::Null, int(ErrorCode::BadArgType));
		}
		
		for (size_t i = 1; i < args.size(); ++i)
		{
			Value v = args[i];
			switch (MAXMIN_ENUM(v.t_vType,is_double))
			{
			case(MAXMIN_ENUM(Value::vType::Double, true)):
				if (v.t_value.as_double > bigval.d)
				{
					biggest = v;
					bigval.d = v.t_value.as_double;
				}
				break;
			case(MAXMIN_ENUM(Value::vType::Double, false)):
				if (v.t_value.as_double > bigval.i)
				{
					biggest = v;
					bigval.d = v.t_value.as_double;
					is_double = true;
				}
				break;
			case(MAXMIN_ENUM(Value::vType::Integer, false)):
				if (v.t_value.as_int > bigval.i)
				{
					biggest = v;
					bigval.i = v.t_value.as_int;
				}
				break;
			case(MAXMIN_ENUM(Value::vType::Integer, true)):
				if (v.t_value.as_int > bigval.d)
				{
					biggest = v;
					bigval.i = v.t_value.as_int;
					is_double = false;
				}
				break;
			default: // Bools aren't allowed here, piss off
				return Value(Value::vType::Null, int(ErrorCode::BadArgType));
			}
		}
		return biggest;
	}));
	NATIVE_FUNC("min")//FIXME: Use BinaryOperation(bOp::LessThan) instead of C++'s less-than operator, for overloading and not-writing-things-in-many-places.
	{
		if (args.size() == 0)
			return Value();

		Value smallest = args[0];
		union
		{
			double d;
			Value::JoaoInt i;
		}smallval;
		bool is_double;
		switch (smallest.t_vType)
		{
		case(Value::vType::Double):
			smallval.d = smallest.t_value.as_double;
			is_double = true;
		case(Value::vType::Integer):
			smallval.i = smallest.t_value.as_int;
			is_double = false;
		default:
			return Value(Value::vType::Null, int(ErrorCode::BadArgType));
		}

		for (size_t i = 1; i < args.size(); ++i)
		{
			Value v = args[i];
			switch (MAXMIN_ENUM(v.t_vType,is_double))
			{
			case(MAXMIN_ENUM(Value::vType::Double, true)):
				if (v.t_value.as_double < smallval.d)
				{
					smallest = v;
					smallval.d = v.t_value.as_double;
				}
				break;
			case(MAXMIN_ENUM(Value::vType::Double, false)):
				if (v.t_value.as_double < smallval.i)
				{
					smallest = v;
					smallval.d = v.t_value.as_double;
					is_double = true;
				}
				break;
			case(MAXMIN_ENUM(Value::vType::Integer, false)):
				if (v.t_value.as_int < smallval.i)
				{
					smallest = v;
					smallval.i = v.t_value.as_int;
				}
				break;
			case(MAXMIN_ENUM(Value::vType::Integer, true)):
				if (v.t_value.as_int < smallval.d)
				{
					smallest = v;
					smallval.i = v.t_value.as_int;
					is_double = false;
				}
				break;
			default: // Bools aren't allowed here, piss off
				return Value(Value::vType::Null, int(ErrorCode::BadArgType));
			}
		}
		return smallest;
	}));
	NATIVE_FUNC("rad")
	{
		Value arg = args[0];
		switch (arg.t_vType)
		{
		case(Value::vType::Double):
			return Value(arg.t_value.as_double * (PI / 180));
		case(Value::vType::Integer):
			return Value(arg.t_value.as_int * (PI / 180));
		case(Value::vType::Bool):
			return Value(arg.t_value.as_bool * (PI / 180));
		default:
			return Value(Value::vType::Null, int(ErrorCode::BadArgType));
		}
	}));

	NATIVE_FUNC("random")
	{
		bool no_args;

		Value::JoaoInt lower;
		Value::JoaoInt upper;
		switch (args.size())
		{
		default:
		case(2):
			switch (args[1].t_vType)
			{
			case(Value::vType::Double):
				upper = static_cast<Value::JoaoInt>(args[1].t_value.as_double);
				break;
			case(Value::vType::Integer):
				upper = args[1].t_value.as_int;
				break;
			default:
				return Value(Value::vType::Null, int(ErrorCode::BadArgType));
			}
			switch (args[0].t_vType)
			{
			case(Value::vType::Double):
				lower = static_cast<Value::JoaoInt>(args[0].t_value.as_double);
				break;
			case(Value::vType::Integer):
				lower = args[0].t_value.as_int;
				break;
			default:
				return Value(Value::vType::Null, int(ErrorCode::BadArgType));
			}
			no_args = false;
			break;
		case(1):
			switch (args[0].t_vType)
			{
			case(Value::vType::Double):
				upper = static_cast<Value::JoaoInt>(args[0].t_value.as_double);
				break;
			case(Value::vType::Integer):
				upper = args[0].t_value.as_int;
				break;
			default:
				return Value(Value::vType::Null, int(ErrorCode::BadArgType));
			}
			lower = 1;
			no_args = false;
			break;
		case(0):
			no_args = true;
		}

		if (no_args)
			return Value(((double)rand() / (RAND_MAX)));

		return Value((rand() % (upper + 1 - lower)) + lower);
	}));

	NATIVE_FUNC("randomseed")
	{
		if(!args.size())
			return Value(Value::vType::Null, int(ErrorCode::NotEnoughArgs));

		srand(reinterpret_cast<uint64_t>(args[0].t_value.as_object_ptr)); // Who cares

		return Value();
	}));

	definedFunctions["round"] = static_cast<Function*>(new NativeFunction("round", math::round)); // FIXME: Weird, should have its own define or... something

	NATIVE_FUNC("sin")
	{
		Value arg = args[0];
		switch (arg.t_vType)
		{
		case(Value::vType::Double):
			return Value(sin(arg.t_value.as_double));
		case(Value::vType::Integer):
			return Value(sin(arg.t_value.as_int));
		case(Value::vType::Bool):
			return Value(sin(arg.t_value.as_bool));
		default:
			return Value(Value::vType::Null, int(ErrorCode::BadArgType));
		}
	}));
	NATIVE_FUNC("sqrt")
	{
		Value arg = args[0];
		switch (arg.t_vType)
		{
		case(Value::vType::Double):
			return Value(sqrt(arg.t_value.as_double));
		case(Value::vType::Integer):
			return Value(sqrt(arg.t_value.as_int));
		case(Value::vType::Bool):
			return Value(sqrt(arg.t_value.as_bool));
		default:
			return Value(Value::vType::Null, int(ErrorCode::BadArgType));
		}
	}));
	NATIVE_FUNC("tan")
	{
		Value arg = args[0];
		switch (arg.t_vType)
		{
		case(Value::vType::Double):
			return Value(tan(arg.t_value.as_double));
		case(Value::vType::Integer):
			return Value(tan(arg.t_value.as_int));
		case(Value::vType::Bool):
			return Value(tan(arg.t_value.as_bool));
		default:
			return Value(Value::vType::Null, int(ErrorCode::BadArgType));
		}
	}));
	NATIVE_FUNC("ult")
	{
		Value lhs = args[0];
		Value rhs = args[1];

		if(lhs.t_vType != Value::vType::Integer || rhs.t_vType != Value::vType::Integer)
			return Value(Value::vType::Null, int(ErrorCode::BadArgType));

		return Value(static_cast<uint32_t>(lhs.t_value.as_int) < static_cast<uint32_t>(rhs.t_value.as_int));
	}));
}

#undef NATIVE_FUNC
#undef PI
#undef MATH_E