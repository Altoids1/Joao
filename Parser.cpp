#include "Parser.h"

#define SYMBOL_ENUMS(a,b) ((a << 9) | b)

void Program::construct_natives()
{
	//TEXT MANIPULATION
	definedFunctions["print"] = &NativeFunction("print", [](std::vector<Value> args)
	{
		if (args.size())
		{
			std::cout << args[0].to_string();
			for (int i = 1; i < args.size(); ++i)
			{
				std::cout << '\t' << args[i].to_string();
			}
		}
		std::cout << "\n";
		return Value();
	});
	definedFunctions["tostring"] = &NativeFunction("tostring", [](std::vector<Value> args)
	{
		return Value(args[0].to_string());
	});
	definedFunctions["void_stellakafuhparenthessisluaunderscorestatewithacapitalscommaluaunderscoreallocfvoidstarud"] = &NativeFunction("void_stellakafuhparenthessisluaunderscorestatewithacapitalscommaluaunderscoreallocfvoidstarud", [](std::vector<Value> args)
	{
		return Value(7);
	});

	//MATHEMATICS
	definedFunctions["sqrt"] = &NativeFunction("sqrt", [](std::vector<Value> args)
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
	definedFunctions["sin"] = &NativeFunction("sin", [](std::vector<Value> args)
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
	definedFunctions["cos"] = &NativeFunction("cos", [](std::vector<Value> args)
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
}
