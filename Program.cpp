#include "Program.h"
#include "Object.h"

#ifdef __GNUG__
	#include "./nativefuncs/math.cpp"
	#include "./nativefuncs/string.cpp"
	#include "./nativefuncs/table.cpp"
#endif

void Program::construct_natives()
{
	//TEXT MANIPULATION
	definedFunctions["print"] = new NativeFunction("print", [](std::vector<Value> args) // Lua-style print() function
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
	definedFunctions["tostring"] = new NativeFunction("tostring", [](std::vector<Value> args)
	{
		return Value(args[0].to_string());
	});
	definedFunctions["tointeger"] = new NativeFunction("tointeger", [](std::vector<Value> args)
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
	
	definedFunctions["typeof"] = new NativeFunction("typeof", [](std::vector<Value> args)
	{
		if(!args.size())
			return Value(Value::vType::Null, int(ErrorCode::NotEnoughArgs));
		Value arg = args[0];

		std::string str;
		switch (arg.t_vType)
		{
		case(Value::vType::Double):
			str =  "Double";
			break;
		case(Value::vType::Integer):
			str =  "Integer";
			break;
		case(Value::vType::Bool):
			str = "Boolean";
			break;
		case(Value::vType::Object):
			str =  "Object";
			break;
		case(Value::vType::String):
			str =  "String";
			break;
		default:
			str =  "Unknown"; // This is very strange, if it were to happen.
			break;
		}
		return Value(str);
	});
	definedFunctions["istable"] = new NativeFunction("istable", [](std::vector<Value> args)
	{
		if(!args.size())
			return Value(Value::vType::Null, int(ErrorCode::NotEnoughArgs));
		Value arg = args[0];

		switch (arg.t_vType)
		{
		case(Value::vType::Object):
			return Value(arg.t_value.as_object_ptr->is_table());
		default:
			return Value(false);
		}
	});
	definedFunctions["isnull"] = new NativeFunction("isnull", [](std::vector<Value> args)
	{
		if(!args.size())
			return Value(Value::vType::Null, int(ErrorCode::NotEnoughArgs));
		return Value(args[0].t_vType == Value::vType::Null);
	});
	
	definedFunctions["classof"] = new NativeFunction("classof", [](std::vector<Value> args)
	{
		if(!args.size())
			return Value(Value::vType::Null, int(ErrorCode::NotEnoughArgs));
		Value arg = args[0];

		if(arg.t_vType != Value::vType::Object)
			return Value(Value::vType::Null, int(ErrorCode::BadArgType));

		return Value(arg.t_value.as_object_ptr->object_type);
	});
	
	definedFunctions["void_stellakafuhparenthessisluaunderscorestatewithacapitalscommaluaunderscoreallocfvoidstarud"] = new NativeFunction("void_stellakafuhparenthessisluaunderscorestatewithacapitalscommaluaunderscoreallocfvoidstarud", [](std::vector<Value> args)
	{
		return Value(7); // No I'm never removing this function fuck you
	});



	construct_math_library();
	construct_string_library();
	construct_table_library();
}
