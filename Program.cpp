#include "Program.h"

void Program::construct_natives()
{
	//TEXT MANIPULATION
	definedFunctions["print"] = &NativeFunction("print", [](std::vector<Value> args) // Lua-style print() function
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
	definedFunctions["tointeger"] = &NativeFunction("tointeger", [](std::vector<Value> args)
	{
		if(!args.size())
			return Value(Value::vType::Null, int(ErrorCode::NotEnoughArgs));
		Value arg = args[0];

		switch (arg.t_vType)
		{
		case(Value::vType::Double):
			return Value(int(arg.t_value.as_double));
		case(Value::vType::Integer):
			return arg;
		case(Value::vType::Bool):
			if (arg.t_value.as_bool)
				return Value(1);
			else
				return Value(0);
		default: // FIXME: Add string-to-int
			return Value(Value::vType::Null, int(ErrorCode::BadArgType));
		}
		return Value(args[0].to_string());
	});
	definedFunctions["void_stellakafuhparenthessisluaunderscorestatewithacapitalscommaluaunderscoreallocfvoidstarud"] = &NativeFunction("void_stellakafuhparenthessisluaunderscorestatewithacapitalscommaluaunderscoreallocfvoidstarud", [](std::vector<Value> args)
	{
		return Value(7);
	});



	construct_math_library();
	construct_string_library();
	construct_table_library();
}
