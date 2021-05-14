#include "../Program.h"
#include "../Object.h"
#include "../Table.h"

ObjectType* Program::construct_table_library()
{
	ObjectType* table = new ObjectType("/table");
	table->is_table_type = true;
	
	table->set_typemethod_raw("#constructor",new NativeMethod("#constructor",[](std::vector<Value> args, Object* obj){
		
		Table* t = static_cast<Table*>(obj);

		for(size_t i = 0; i < args.size(); ++i)
		{
			t->t_array.push_back(args[i]);
		}

		return Value(obj); // If anything.
	}));
	table->set_typemethod_raw("implode",new NativeMethod("implode",[](std::vector<Value> args, Object* obj){
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


	table->set_typemethod_raw("insert",new NativeMethod("insert",[](std::vector<Value> args, Object* obj){
		if(args.size() < 2)
			return Value(Value::vType::Null, int(ErrorCode::NotEnoughArgs));
		if(args[0].t_vType != Value::vType::Integer)
			return Value(Value::vType::Null, int(ErrorCode::BadArgType));
		
		size_t index = args[0].t_value.as_int;

		//This tries to mimic some of the behavior of at_set() w/o explicitly calling it
		//FIXME: This should really all use the same function tbh

		Table* t = static_cast<Table*>(obj);

		if(index > t->t_array.size())
		{
			t->t_array.resize(index,Value());
			t->t_array.push_back(args[1]);
		}
		else if(index ==  t->t_array.size())
		{
			t->t_array.push_back(args[1]);
		}
		else
		{
			t->t_array.insert(t->t_array.begin() + index,args[1]);
		}
		return Value();
	}));
	table->set_typemethod_raw("remove",new NativeMethod("remove",[](std::vector<Value> args, Object* obj){
		if(args.size() < 1)
			return Value(Value::vType::Null, int(ErrorCode::NotEnoughArgs));
		if(args[0].t_vType != Value::vType::Integer)
			return Value(Value::vType::Null, int(ErrorCode::BadArgType));
		
		size_t index = args[0].t_value.as_int;

		//This tries to mimic some of the behavior of at_set() w/o explicitly calling it
		//FIXME: This should really all use the same function tbh

		Table* t = static_cast<Table*>(obj);

		if(index >= t->t_array.size())
		{
			return Value();
		}

		t->t_array.erase(t->t_array.begin() + index);
		return Value();
	}));

	return table;
}

#undef NATIVE_FUNC_TABLE
