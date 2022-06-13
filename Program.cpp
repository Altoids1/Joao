#include "Program.h"
#include "Object.h"
#include "AST.hpp"

#define NATIVE_FUNC(name) definedFunctions[ name ] = static_cast<Function*>(new NativeFunction( name , [](Interpreter& interp, const std::vector<Value>& args)

HashTable<std::string,ObjectType*> Program::construct_natives()
{
	globals.table["__VERSION"] = Value(std::string(VERSION_STRING));
	globals.table["__VERSION_MAJOR"] = Value(VERSION_MAJOR);
	globals.table["__VERSION_MINOR"] = Value(VERSION_MINOR);
	globals.table["__VERSION_PATCH"] = Value(VERSION_PATCH);

	//TEXT MANIPULATION
	NATIVE_FUNC("print") // Lua-style print() function
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
	}));
	NATIVE_FUNC("input") // Analogous to Lua's io.read() function.
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
	}));
	NATIVE_FUNC("tostring")
	{
		return Value(args[0].to_string());
	}));
	NATIVE_FUNC("tointeger")
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
	}));
	
	NATIVE_FUNC("typeof")
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
	}));
	NATIVE_FUNC("istable")
	{
		if(!args.size())
			return Value(Value::vType::Null, int(ErrorCode::NotEnoughArgs));
		const Value& arg = args[0];

		switch (arg.t_vType)
		{
		case(Value::vType::Object):
			return Value(arg.t_value.as_object_ptr->is_table());
		default:
			return Value(false);
		}
	}));
	NATIVE_FUNC("isnull")
	{
		if(!args.size())
			return Value(Value::vType::Null, int(ErrorCode::NotEnoughArgs));
		return Value(args[0].t_vType == Value::vType::Null);
	}));
	
	NATIVE_FUNC("classof")
	{
		if(!args.size())
			return Value(Value::vType::Null, int(ErrorCode::NotEnoughArgs));
		Value arg = args[0];

		if(arg.t_vType != Value::vType::Object)
			return Value(Value::vType::Null, int(ErrorCode::BadArgType));

		return Value(arg.t_value.as_object_ptr->object_type.to_string());
	}));
	
	NATIVE_FUNC("void_stellakafuhparenthessisluaunderscorestatewithacapitalscommaluaunderscoreallocfvoidstarud")
	{
		return Value(7); // No I'm never removing this function fuck you
	}));

	HashTable<std::string, ObjectType*> cooked_classes;

	construct_math_library();
	construct_string_library();
	cooked_classes["/table"] = construct_table_library();
#ifndef JOAO_SAFE
	cooked_classes["/file"] = construct_file_library();
#endif
	cooked_classes["/error"] = construct_error_library();

	return cooked_classes;
}
