#include "../Program.h"
#include "../Object.h"
#include "../Table.h"
#include "../AST.hpp"

ObjectType* Program::construct_table_library()
{
	ObjectType* table = new ObjectType("/table");
	table->is_table_type = true;
	
	table->set_typemethod_raw("#constructor",new NativeMethod("#constructor",[](const std::vector<Value>& args, Object* obj){
		
		Table* t = static_cast<Table*>(obj);

		for(size_t i = 0; i < args.size(); ++i)
		{
			t->t_array.push_back(args[i]);
		}

		return Value(true); // If anything.
	}));
	table->set_typemethod_raw("implode",new NativeMethod("implode",[](const std::vector<Value>& args, Object* obj){
		Table* t = static_cast<Table*>(obj);

		std::string sep = ", ";
		size_t start = 0;
		size_t stop = t->t_array.size() - 1;

		switch(args.size())
		{
		default:
		case(3):
			if(args[2].t_vType != Value::vType::Integer)
				return Value(Value::vType::Null, int(ErrorCode::BadArgType));
			stop = args[2].t_value.as_int;
			//Falls over into the other arguments
		case(2):
			if(args[1].t_vType != Value::vType::Integer)
				return Value(Value::vType::Null, int(ErrorCode::BadArgType));
			start = args[1].t_value.as_int;
			//Falls over into the other arguments
		case(1):
			sep = args[0].to_string();
		case(0):
			break;
		}

		if (!(t->t_array.size()) || start >= t->t_array.size())
			return Value("");
		
		std::string result = t->t_array[start].to_string();

		for(++start; start <= stop && start < t->t_array.size(); ++start)
		{
			result += sep + t->t_array[start].to_string();
		}
		return Value(result);
	}));

	table->set_typemethod_raw("pick", new NativeMethod("pick", [](const std::vector<Value>& args, Object* obj) {
		Table* t = static_cast<Table*>(obj);

		if (!t->t_array.size())
			return Value();

		return Value(t->t_array[rand() % t->t_array.size()]);
	}));

	definedFunctions["pick"] = new NativeFunction("pick", [](const std::vector<Value>& args)
		{
			if (!args.size())
				return Value();
			return Value(args[rand() % args.size()]);
		});

	table->set_typemethod_raw("insert", new NativeMethod("insert", [](const std::vector<Value>& args, Object* obj) {
		if (args.size() < 2)
			return Value(Value::vType::Null, int(ErrorCode::NotEnoughArgs));
		const Value& vindex = args[0];
		Value::JoaoInt index = 0;
		Table* table = static_cast<Table*>(obj);
		switch (vindex.t_vType)
		{
		default:
			return Value(Value::vType::Null, int(ErrorCode::BadArgType));
		case(Value::vType::Double):
			index = math::round({ vindex.t_value.as_double }).t_value.as_int;
			[[fallthrough]];
		case(Value::vType::Integer):
			index = index || vindex.t_value.as_int; // Best I could come up with
			//Since we're in /table/insert(), and not Table::at_set() or whatever,
			//We have to make a point of, like, actually inserting into t_array if necessary.
			if (index >= 0)
			{
				auto& arr = table->t_array;
				if (index < arr.size()) // if we're trying to insert at a place where an element already exists
				{
					auto& hash = table->t_hash;
					//Before we do this, imagine that there's an index [9] present in the hashtable, and the array only goes from [0] to [8].
					//Inserting naïvely in that case would clobber the 9th element, which is a bad.

					//talloc() tries to prevent this, but it's still possible, so lets do some checks first.
					while (hash.count(std::hash<Value::JoaoInt>()(arr.size()))) // If this is the case
					{ // take that element and actually put it on the fucking array, I guess
						Value::JoaoInt hash_index = std::hash<Value::JoaoInt>()(arr.size());
						arr.push_back(std::move(hash.at(hash_index)));
						hash.erase(hash_index);
					}

					arr.insert(arr.begin() + index, args[1]);
					return Value(); // Skips a lot of the silliness of talloc(), which is nice, even if insert() is expensive.
				}
			}
			//Otherwise, fallthrough to just using talloc() like normal.
			[[fallthrough]];
		case(Value::vType::String):
			if (table->at_set_raw(vindex, args[1]))
			{
				return Value(Value::vType::Null, int(ErrorCode::FailedOperation)); // I'unno.
			}
			return Value();
		}
	}));

	table->set_typemethod_raw("remove",new NativeMethod("remove",[](const std::vector<Value>& args, Object* obj){
		if(args.size() < 1)
			return Value(Value::vType::Null, int(ErrorCode::NotEnoughArgs));
		const Value& vindex = args[0];
		switch (vindex.t_vType)
		{
		default:
			return Value(Value::vType::Null, int(ErrorCode::BadArgType));
		case(Value::vType::Integer):
		case(Value::vType::String):
		case(Value::vType::Double):
			static_cast<Table*>(obj)->tfree(vindex);
			return Value();
		}
	}));

	return table;
}

#undef NATIVE_FUNC_TABLE
