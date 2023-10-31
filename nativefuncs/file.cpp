#include "../Program.h"
#include "../Object.h"
#include "../Table.h"
#include "../AST.hpp"

//#define NATIVE_FUNC_TABLE(name) static_cast<Function*>(new NativeFunction( name , [](std::vector<Value> args)

ObjectType* Program::construct_file_library()
{
	Metatable* __mt = new Metatable();

	APPENDMETHOD(__mt,"open",
	[](std::vector<Value> args, Object* obj)
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
		PolyTable& privates = obj->get_privates();
		std::fstream* our_file = privates.lazy_at<std::fstream>("FSTREAM");
		if (our_file) // Silently close any file this file object currently has open when we open a new file
		{
			privates.data.remove("FSTREAM");
		}

		std::string filename = *first.t_value.as_string_ptr;

		privates.insert<std::fstream>("FSTREAM");
		
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
		
		return Value(our_file->good());
	});

	/*
	Returns a table with its array part filled with the lines of this file.
	*/
	APPENDMETHOD(__mt, "lines",
	[]([[maybe_unused]] std::vector<Value> args, Object* obj)
	{
		PolyTable& privates = obj->get_privates();
		std::fstream* our_file = privates.lazy_at<std::fstream>("FSTREAM");
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
	});


	APPENDMETHOD(__mt, "write",
	[](std::vector<Value> args, Object* obj)
	{
		PolyTable& privates = obj->get_privates();
		std::fstream* our_file = privates.lazy_at<std::fstream>("FSTREAM");
		if (!our_file)
			return Value();


		for (size_t i = 0; i < args.size(); ++i)
		{
			(*our_file) << args[i].to_string();
		}

		return Value(our_file->good());
	});


	APPENDMETHOD(__mt,"close",
	[]([[maybe_unused]] std::vector<Value> args, Object* obj)
	{
		PolyTable& privates = obj->get_privates();
		std::fstream* our_file = privates.lazy_at<std::fstream>("FSTREAM");
		if (!our_file)
			return Value();

		if (!our_file->is_open())
			return Value();

		our_file->close();
		privates.data.erase("FSTREAM");
		return Value(true);
	});

	return (new ObjectType("/file", __mt, true));
}