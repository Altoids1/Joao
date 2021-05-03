#include "../Program.h"

#define NATIVE_FUNC(name) definedFunctions[##name] = static_cast<Function*>(new NativeFunction(##name, [](std::vector<Value> args)

void Program::construct_string_library()
{
	NATIVE_FUNC("find") //TODO: Implement a Regex type with a basic regex engine. Someday. Maybe.
	{
		if(args.size() < 2)
			return Value(Value::vType::Null, int(ErrorCode::NotEnoughArgs));

		Value hay = args[0];
		Value needle = args[1];
		if(hay.t_vType != Value::vType::String || needle.t_vType != Value::vType::String)
			return Value(Value::vType::Null, int(ErrorCode::BadArgType));
		
		size_t cpp_result = hay.t_value.as_string_ptr->find(*needle.t_value.as_string_ptr);
		if (cpp_result == std::string::npos)
			return Value();

		return Value(cpp_result);
	}));

	NATIVE_FUNC("rep")
	{
		if (args.size() < 2)
			return Value(Value::vType::Null, int(ErrorCode::NotEnoughArgs));

		Value repetition_legitimizes = args[0];
		Value repnum = args[1];

		if (repetition_legitimizes.t_vType != Value::vType::String || repnum.t_vType != Value::vType::Integer)
			return Value(Value::vType::Null, int(ErrorCode::BadArgType));

		std::string str = *repetition_legitimizes.t_value.as_string_ptr;

		int64_t times = repnum.t_value.as_int;
		for (int64_t i = 1; i < times; ++i)
		{
			str += *repetition_legitimizes.t_value.as_string_ptr;
		}

		return Value(str);
	}));

	NATIVE_FUNC("reverse")
	{
		if (args.size() < 1)
			return Value(Value::vType::Null, int(ErrorCode::NotEnoughArgs));

		Value record = args[0];

		if (record.t_vType != Value::vType::String)
			return Value(Value::vType::Null, int(ErrorCode::BadArgType));

		std::string oldstr = *record.t_value.as_string_ptr;
		std::string str = "";

		for (auto it = oldstr.rbegin(); it != oldstr.rend(); ++it)
		{
			str.push_back(*it);
		}

		return Value(str);
	}));

	NATIVE_FUNC("substr")
	{
		if (args.size() < 3)
			return Value(Value::vType::Null, int(ErrorCode::NotEnoughArgs));

		Value hay = args[0];
		Value start = args[1];
		Value stop = args[2];

		if (hay.t_vType != Value::vType::String || start.t_vType != Value::vType::Integer || stop.t_vType != Value::vType::Integer)
			return Value(Value::vType::Null, int(ErrorCode::BadArgType));

		std::string str = *hay.t_value.as_string_ptr;

		return Value(str.substr(start.t_value.as_int,stop.t_value.as_int - start.t_value.as_int + 1));
	}));
}


#undef NATIVE_FUNC
