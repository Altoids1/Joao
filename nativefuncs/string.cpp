#include "../Program.h"
#include "../AST.hpp"
#include "../Object.h"
#include "../Interpreter.h"

#define NATIVE_FUNC(name) definedFunctions[ name ] = static_cast<Function*>(new NativeFunction( name , [](Interpreter& interp, const std::vector<Value>& args)

void Program::construct_string_library()
{
	NATIVE_FUNC("find") //TODO: Implement a Regex type with a basic regex engine. Someday. Maybe.
	{
		if(args.size() < 2)
			return Value(Value::vType::Null, int(ErrorCode::NotEnoughArgs));

		const Value& hay = args[0];
		const Value& needle = args[1];
		if(hay.t_vType != Value::vType::String || needle.t_vType != Value::vType::String)
			return Value(Value::vType::Null, int(ErrorCode::BadArgType));
		
		size_t cpp_result = hay.t_value.as_string_ptr->find(*needle.t_value.as_string_ptr);
		if (cpp_result == std::string::npos)
			return Value();

		return Value(cpp_result);
	}));

	NATIVE_FUNC("replace")
	{
		if (args.size() < 3)
			return Value(Value::vType::Null, int(ErrorCode::NotEnoughArgs));
		const Value& hay = args[0];
		const Value& needle = args[1];
		const Value& better_hay = args[2];
		if (hay.t_vType != Value::vType::String || needle.t_vType != Value::vType::String || better_hay.t_vType != Value::vType::String)
			return Value(Value::vType::Null, int(ErrorCode::BadArgType));

		const std::string& hay_str = *hay.t_value.as_string_ptr;
		const std::string& needle_str = *needle.t_value.as_string_ptr;
		const std::string& better_hay_str = *better_hay.t_value.as_string_ptr;
		const int needle_len = needle_str.size();
		if (hay_str.find(needle_str) == std::string::npos)
			return hay;
		std::string newstr = hay_str;
		size_t position = 0;
#ifdef JOAO_SAFE
		for(int i = 0; i < MAX_REPLACEMENTS_ALLOWED; ++i) // It just returning the partially-replaced string is... fine, I guess.
#else
		while (true)
#endif
		{
			auto pos = newstr.find(needle_str, position);
			if (pos == std::string::npos)
				break;
			newstr.replace(pos, needle_len, better_hay_str);
			position = pos + better_hay_str.size(); // If size is one, then look at the pos+1 position next, I guess!
			if (position >= newstr.size())
				break;
		}
		return Value(newstr);
	}));

	NATIVE_FUNC("rep")
	{
		if (args.size() < 2)
			return Value(Value::vType::Null, int(ErrorCode::NotEnoughArgs));

		const Value& repetition_legitimizes = args[0];
		const Value& repnum = args[1];

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

		const Value& record = args[0];

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

		const Value& hay = args[0];
		const Value& start = args[1];
		const Value& stop = args[2];

		if (hay.t_vType != Value::vType::String || start.t_vType != Value::vType::Integer || stop.t_vType != Value::vType::Integer)
			return Value(Value::vType::Null, int(ErrorCode::BadArgType));

		std::string str = *hay.t_value.as_string_ptr;

		return Value(str.substr(start.t_value.as_int,stop.t_value.as_int - start.t_value.as_int + 1));
	}));

	NATIVE_FUNC("explode")
	{
		char sep = ' ';
		std::string* str_ptr;
		switch (args.size())
		{
		default:
		case(2):
			if(args[1].t_vType != Value::vType::String || args[1].t_value.as_string_ptr->empty())
				return Value(Value::vType::Null, int(ErrorCode::BadArgType));
			sep = args[1].t_value.as_string_ptr->at(0); // FIXME: Learn to support non-character separators.
			[[fallthrough]];
		case(1):
			if (args[0].t_vType != Value::vType::String)
				return Value(Value::vType::Null, int(ErrorCode::BadArgType));
			str_ptr = args[0].t_value.as_string_ptr;
			break;
		case(0):
			return Value(Value::vType::Null, int(ErrorCode::NotEnoughArgs));
		}

		const std::string& ref_str = *str_ptr;
		if (ref_str.empty()) return Value(interp.makeObject("/table", {}, nullptr));
		std::vector<Value> new_arr;
		size_t pos = 0;
		size_t end = ref_str.size();
#ifdef JOAO_SAFE
		for (int i = 0; i < MAX_REPLACEMENTS_ALLOWED; ++i) // It just returning the partially-replaced string is... fine, I guess.
#else
		while (pos < end)
#endif
		{
			size_t new_pos = ref_str.find(sep, pos);
			if (new_pos == std::string::npos)
			{
				new_arr.push_back(Value(ref_str.substr(pos,std::string::npos)));
				break;
			}
			new_arr.push_back(Value(ref_str.substr(pos, new_pos-pos)));
			pos = new_pos + 1; // Move to the next character
		}

		return Value(interp.makeObject("/table",new_arr,nullptr));
	}));
}


#undef NATIVE_FUNC
