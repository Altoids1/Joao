#include "../Program.h"
#include "../Object.h"
#include "../Table.h"

//#define NATIVE_FUNC_TABLE(name) static_cast<Function*>(new NativeFunction( name , [](std::vector<Value> args)

ObjectType* Program::construct_file_library()
{
	Metatable* __mt = new Metatable();

	enum class PrivIndex : size_t
	{
		FSTREAM
	};

	__mt->append_method("open", new NativeMethod("open", [](std::vector<Value> args, Object* obj)
	{
		size_t argnum = args.size();

		if (!argnum)
			return Value(Value::vType::Null, static_cast<int>(ErrorCode::NotEnoughArgs));

		Value first = args[0];
		if(first.t_vType != Value::vType::String)
			return Value(Value::vType::Null, static_cast<int>(ErrorCode::BadArgType));

		bool blank_it = false;

		if (argnum > 1)
		{
			const Value& second = args[1];
			blank_it = static_cast<bool>(second);
		}

		Metatable* mt = obj->get_metatable();
		std::fstream* our_file = static_cast<std::fstream*>(mt->get_private(static_cast<size_t>(PrivIndex::FSTREAM)));
		if (our_file) // Silently close any file this file object currently has open when we open a new file
		{
			our_file->close();
			delete our_file;
		}

		std::string filename = *first.t_value.as_string_ptr;

		

		our_file = new std::fstream();
		try
		{
			if(blank_it)
				our_file->open(filename, std::fstream::in | std::fstream::out | std::fstream::trunc);
			else
				our_file->open(filename, std::fstream::in | std::fstream::out);
			
			if(!our_file->good())
			{
				our_file->close();
				throw;
			}
		}
		catch(...)
		{
			return Value(Value::vType::Null, static_cast<int>(ErrorCode::BadCall));
		}
		mt->set_private(static_cast<size_t>(PrivIndex::FSTREAM), our_file);
		return Value(our_file->good());
	}));

	/*
	Returns a table with its array part filled with the lines of this file.
	*/
	__mt->append_method("lines", new NativeMethod("lines", [](std::vector<Value> args, Object* obj)
	{
		Metatable* mt = obj->get_metatable();
		std::fstream* our_file = static_cast<std::fstream*>(mt->get_private(static_cast<size_t>(PrivIndex::FSTREAM)));
		if (!our_file)
			return Value();

		Table* tuh = new Table("/table", {}, {}); // RAW-ass motherfucking table since we can't really call interp.makeBaseTable() in this lambda's scope
		
		std::string str;

		for(int index = 0; !our_file->eof(); ++index)
		{

			std::getline(*our_file,str);
			tuh->t_array.push_back(Value(str));
		}


		return Value(tuh);
	}));
	__mt->append_method("write", new NativeMethod("write", [](std::vector<Value> args, Object* obj)
	{
		Metatable* mt = obj->get_metatable();
		std::fstream* our_file = static_cast<std::fstream*>(mt->get_private(static_cast<size_t>(PrivIndex::FSTREAM)));
		if (!our_file)
			return Value();


		for (size_t i = 0; i < args.size(); ++i)
		{
			(*our_file) << args[i].to_string();
		}

		return Value(our_file->good());
	}));
	__mt->append_method("close", new NativeMethod("close", [](std::vector<Value> args, Object* obj)
	{
		Metatable* mt = obj->get_metatable();
		std::fstream* our_file = static_cast<std::fstream*>(mt->get_private(static_cast<size_t>(PrivIndex::FSTREAM)));
		if (!our_file)
			return Value();

		if (!our_file->is_open())
			return Value();

		our_file->close();

		delete our_file;
		mt->set_private(static_cast<size_t>(PrivIndex::FSTREAM), nullptr);

		return Value(true);
	}));

	return (new ObjectType("/file", __mt));
}