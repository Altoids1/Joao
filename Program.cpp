#include "Program.h"
#include "Object.h"
#include "AST.hpp"

std::unordered_map<std::string,ObjectType*> Program::construct_natives()
{
	globals.table["__VERSION"] = new Value(std::string(VERSION_STRING));
	globals.table["__VERSION_MAJOR"] = new Value(VERSION_MAJOR);
	globals.table["__VERSION_MINOR"] = new Value(VERSION_MINOR);
	globals.table["__VERSION_PATCH"] = new Value(VERSION_PATCH);

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
	definedFunctions["input"] = new NativeFunction("input", [](std::vector<Value> args) // Analogous to Lua's io.read() function.
	{
		if (args.size())
		{
			const Value& arg = args[0];
			if (arg.t_vType == Value::vType::Integer) // Attempts to read in a specific number of characters
			{
				char* str = new char[arg.t_value.as_int + 1];
				std::cin.read(str, arg.t_value.as_int);
				str[arg.t_value.as_int] = '\0'; // Just making absolutely sure that this is null-terminated
				return Value(std::string(str));
			}
		}
		std::string imp;
		std::cin >> imp;
		if(imp.empty())
			return Value();
		return Value(imp);
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
		case(Value::vType::Null):
			str = "Null";
			break;
		case(Value::vType::Function):
			str = "Function";
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

	std::unordered_map<std::string, ObjectType*> cooked_classes;

	construct_math_library();
	construct_string_library();
	cooked_classes["/table"] = construct_table_library();
#ifndef JOAO_SAFE
	cooked_classes["/file"] = construct_file_library();
#endif
	cooked_classes["/error"] = construct_error_library();

	return cooked_classes;
}
