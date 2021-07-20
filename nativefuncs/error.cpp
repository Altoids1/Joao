#include "../Program.h"
#include "../Object.h"

ObjectType* Program::construct_error_library()
{
	ObjectType* error = new ObjectType("/error");

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

	error->set_typemethod_raw("#constructor", new NativeMethod("#constructor", [](std::vector<Value> args, Object* obj) {

		switch (args.size())
		{
		default:
		case(2):
			obj->set_property_raw("what", args[1].to_string());
		case(1):
			if (args[0].t_vType != Value::vType::Integer && args[0].t_vType != Value::vType::Double)
				return Value(Value::vType::Null, int(ErrorCode::BadArgType));
			obj->set_property_raw("errcode", args[0]);
		case(0):
			break;
		}

		return Value(); // If anything.
	}));
	error->set_typemethod_raw("#tostring", new NativeMethod("#tostring", [](std::vector<Value> args, Object* obj) {
		return obj->get_property_raw("what");
	}));

	return error;
}