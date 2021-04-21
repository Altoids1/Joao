#include "../Program.h"

#include <limits>

#define MATH_E 2.71828182845904523536
#define PI 3.14159265358979323846

#define NATIVE_FUNC(name) definedFunctions[##name] = &NativeFunction(##name, [](std::vector<Value> args)

void Program::construct_math_library()
{
	//MATHEMATICS

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
	});
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
	});
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
	});
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
	});
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
	});

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
	});
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
	});
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
	});
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
	});
	NATIVE_FUNC("max")//FIXME: Use BinaryOperation(bOp::Greater) instead of C++'s greater-than operator, for overloading and not-writing-things-in-many-places.
	{
		if (args.size() == 0)
			return Value();

		Value biggest = args[0];
		double bigval; // Yes, I can store Int32s into Float64s without loss of precision. According to Julia just now, typemax(Int32) < maxintfloat(Float64).
		switch (biggest.t_vType)
		{
		case(Value::vType::Double):
			bigval = biggest.t_value.as_double;
		case(Value::vType::Integer):
			bigval = biggest.t_value.as_int;
		default:
			return Value(Value::vType::Null, int(ErrorCode::BadArgType));
		}
		
		for (int i = 1; i < args.size(); ++i)
		{
			Value v = args[i];
			switch (v.t_vType)
			{
			case(Value::vType::Double):
				if (v.t_value.as_double > bigval)
				{
					biggest = v;
					bigval = v.t_value.as_double;
				}
			case(Value::vType::Integer):
				if (v.t_value.as_int > bigval)
				{
					biggest = v;
					bigval = v.t_value.as_int;
				}
			default: // Bools aren't allowed here, piss off
				return Value(Value::vType::Null, int(ErrorCode::BadArgType));
			}
		}
		return biggest;
	});
	NATIVE_FUNC("min")//FIXME: Use BinaryOperation(bOp::LessThan) instead of C++'s less-than operator, for overloading and not-writing-things-in-many-places.
	{
		if (args.size() == 0)
			return Value();

		Value smallest = args[0];
		double smallval; // Yes, I can store Int32s into Float64s without loss of precision. According to Julia just now, typemax(Int32) < maxintfloat(Float64).
		switch (smallest.t_vType)
		{
		case(Value::vType::Double):
			smallval = smallest.t_value.as_double;
		case(Value::vType::Integer):
			smallval = smallest.t_value.as_int;
		default:
			return Value(Value::vType::Null, int(ErrorCode::BadArgType));
		}

		for (int i = 1; i < args.size(); ++i)
		{
			Value v = args[i];
			switch (v.t_vType)
			{
			case(Value::vType::Double):
				if (v.t_value.as_double < smallval)
				{
					smallest = v;
					smallval = v.t_value.as_double;
				}
			case(Value::vType::Integer):
				if (v.t_value.as_int < smallval)
				{
					smallest = v;
					smallval = v.t_value.as_int;
				}
			default: // Bools aren't allowed here, piss off
				return Value(Value::vType::Null, int(ErrorCode::BadArgType));
			}
		}
		return smallest;
	});
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
	});

	//FIXME: Add random number generator functions here!

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
	});
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
	});
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
	});
	NATIVE_FUNC("ult")
	{
		Value lhs = args[0];
		Value rhs = args[1];

		if(lhs.t_vType != Value::vType::Integer || rhs.t_vType != Value::vType::Integer)
			return Value(Value::vType::Null, int(ErrorCode::BadArgType));

		return Value(static_cast<uint32_t>(lhs.t_value.as_int) < static_cast<uint32_t>(rhs.t_value.as_int));
	});
}

#undef NATIVE_FUNC
#undef PI
#undef MATH_E