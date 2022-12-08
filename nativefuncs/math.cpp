#include "../Program.h"
#include "../FailureOr.h"
#include "../AST.hpp"

#include <limits>

#define MATH_E 2.71828182845904523536
#define PI 3.14159265358979323846

#define NATIVE_FUNC(name) definedFunctions[ name ] = static_cast<Function*>(new NativeFunction( name , [](Interpreter& interp, const std::vector<Value>& args)

#define MAXMIN_ENUM(type,thebool) (static_cast<uint16_t>(type) | (static_cast<uint16_t>(thebool) << 8))


Value math::round(const std::vector<Value>& args)
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

//A goofy alternative definition to resolve some awkwardness in how NativeFunctions are implemented.
//FIXME: This is stupid.
Value math::round_safe(Interpreter& interp, const std::vector<Value>& args)
{
	return math::round(args);
}


void Program::construct_math_library()
{
	//MATHEMATICS

	//GLOBAL VALUES
	globals.table["PI"] = Value(PI);
	globals.table["EULER"] = Value(MATH_E);

	//GLOBAL FUNCTIONS
	NATIVE_FUNC("abs")
	{
		const Value& arg = args[0];
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
		const Value& arg = args[0];
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
		const Value& arg = args[0];
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
		const Value& arg = args[0];
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
		const Value& arg = args[0];
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
		const Value& arg = args[0];
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
		const Value& arg = args[0];
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
		const Value& arg = args[0];

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
		const Value& arg = args[0];
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
	NATIVE_FUNC("log")
	{
		if (args.size() == 0)
			return Value();

		const Value& arg = args[0];
		switch (arg.t_vType)
		{
		case(Value::vType::Double):
			return Value(log(arg.t_value.as_double));
		case(Value::vType::Integer):
			return Value(log(arg.t_value.as_int));
		case(Value::vType::Bool):
			return Value(log(arg.t_value.as_bool));
		default:
			return Value(Value::vType::Null, int(ErrorCode::BadArgType));
		}
	}));
	NATIVE_FUNC("log10")
	{
		if (args.size() == 0)
			return Value();

		const Value& arg = args[0];
		switch (arg.t_vType)
		{
		case(Value::vType::Double):
			return Value(log10(arg.t_value.as_double));
		case(Value::vType::Integer):
			return Value(log10(arg.t_value.as_int));
		case(Value::vType::Bool):
			return Value(log10(arg.t_value.as_bool));
		default:
			return Value(Value::vType::Null, int(ErrorCode::BadArgType));
		}
	}));
	NATIVE_FUNC("max")
	{
		if (args.size() == 0)
			return Value();

		Value biggest = args[0];
		
		for (size_t i = 1; i < args.size(); ++i)
		{
			const Value& v = args[i];
			Value result = BinaryExpression::BinaryOperation(biggest, v, BinaryExpression::bOps::LessThan).get_or_throw(interp);
			if(result) // if(biggest < v)
				biggest = v;
		}
		return biggest;
	}));
	NATIVE_FUNC("min")
	{
		if (args.size() == 0)
			return Value();

		Value smallest = args[0];
		
		for (size_t i = 1; i < args.size(); ++i)
		{
			const Value& v = args[i];
			Value result = BinaryExpression::BinaryOperation(smallest, v, BinaryExpression::bOps::Greater).get_or_throw(interp);
			if(result) // if(smaller > v)
				smallest = v;
		}
		return smallest;
	}));
	NATIVE_FUNC("rad")
	{
		const Value& arg = args[0];
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

	definedFunctions["round"] = static_cast<Function*>(new NativeFunction("round", math::round_safe)); // FIXME: Weird, should have its own define or... something

	NATIVE_FUNC("sin")
	{
		const Value& arg = args[0];
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
		const Value& arg = args[0];
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
		const Value& arg = args[0];
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
		const Value& lhs = args[0];
		const Value& rhs = args[1];

		if(lhs.t_vType != Value::vType::Integer || rhs.t_vType != Value::vType::Integer)
			return Value(Value::vType::Null, int(ErrorCode::BadArgType));

		return Value(static_cast<uint32_t>(lhs.t_value.as_int) < static_cast<uint32_t>(rhs.t_value.as_int));
	}));
}

#undef NATIVE_FUNC
#undef PI
#undef MATH_E