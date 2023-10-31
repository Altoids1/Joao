#include "../Program.h"
#include "../Object.h"
#include "../AST.hpp"

ObjectType* Program::construct_error_library()
{
	ObjectType* error = new ObjectType("/error", new Metatable(), true);
	error->set_typeproperty_raw("what", Value());
	error->set_typeproperty_raw("code", Value());

	/*
	/error
	{
		Number errcode = 0;
		String what = "";
	}
	/error/New(e,c)
	{
		errcode = e;
		what = c;
	}
	/error/to_string()
	{
		return what;
	}
	*/

	NATIVEMETHOD(error,"#constructor",
	[](const std::vector<Value>& args, Object* obj) {

		switch (args.size())
		{
		default:
		case(2):
			if (args[1].t_vType != Value::vType::String)
				return Value(Value::vType::Null, int(ErrorCode::BadArgType));
			obj->set_property_raw("what", args[1]);
			[[fallthrough]];
		case(1):
			if (args[0].t_vType != Value::vType::Integer && args[0].t_vType != Value::vType::Double)
				return Value(Value::vType::Null, int(ErrorCode::BadArgType));
			obj->set_property_raw("code", args[0]);
			[[fallthrough]];
		case(0):
			break;
		}

		return Value(); // If anything.
	});

	NATIVEMETHOD(error,"#tostring",
	[]([[maybe_unused]] const std::vector<Value>& args, Object* obj) {
		return obj->get_property_raw("what");
	});

	return error;
}