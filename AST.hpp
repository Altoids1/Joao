/*
This hiccup of a file is used to store out-of-line templated methods from AST.h;
as a result of the flakey and buggy nature of templated code.

TODO: Make this file go away somehow.
*/
#include "AST.h"
#include "Interpreter.h"

template <typename Lambda>
Value NativeFunction<Lambda>::resolve(Interpreter& interp)
{
	Value result = lambda(interp, t_args);
	if (result.t_vType == Value::vType::Null && result.t_value.as_int)
	{
		switch (result.t_value.as_int)
		{
		case(static_cast<Value::JoaoInt>((ErrorCode::NoError))): // An expected null, function returned successfully.
			break;
		case(static_cast<Value::JoaoInt>(ErrorCode::BadArgType)):
			interp.RuntimeError(this, ErrorCode::BadArgType, "Args of improper type given to NativeFunction!");
			break;
		case(static_cast<Value::JoaoInt>(ErrorCode::NotEnoughArgs)):
			interp.RuntimeError(this, ErrorCode::NotEnoughArgs, "Not enough args provided to NativeFunction!");
			break;
		default:
			interp.RuntimeError(this, "Unknown RuntimeError in NativeFunction!");
		}
	}
	return result; // Woag.
}

template <typename Lambda>
Value NativeMethod<Lambda>::resolve(Interpreter& interp)
{
	if (!obj && !is_static)
		interp.RuntimeError(this, "Cannot call NativeMethod without an Object!");

	Value result = lambda(t_args, obj);
	if (result.t_vType == Value::vType::Null && result.t_value.as_int)
	{
		switch (result.t_value.as_int)
		{
		case(static_cast<Value::JoaoInt>(ErrorCode::NoError)): // An expected null, function returned successfully.
			break;
		case(static_cast<Value::JoaoInt>(ErrorCode::BadArgType)):
			interp.RuntimeError(this, ErrorCode::BadArgType, "Args of improper type given to NativeMethod!");
			break;
		case(static_cast<Value::JoaoInt>(ErrorCode::NotEnoughArgs)):
			interp.RuntimeError(this, ErrorCode::NotEnoughArgs, "Not enough args provided to NativeMethod!");
			break;
		default:
			interp.RuntimeError(this, "Unknown RuntimeError in NativeMethod!");
		}
	}
	return result; // Woag.
}

#if defined(__GNUC__) && __GNUC__ <= 8
//This macro exists, in part, to get around a bug in GCC 8.1,
//that caused it to be unable to do type deduction of lambda expressions for template-class constructors.
//That whole jumble of words is just too jumbly for it to understand, despite it definitely being compliant with C++17 spec.
//Why do I not just move to a different compiler?
//Because Github Actions has 8.1 as their default C++ compiler on their Windows build with no plans to increment at all.
//Despite it being years stale. Kill me.
#define NATIVEMETHOD(objtype,func,lambda) { auto l = lambda; objtype->set_typemethod_raw(func, new NativeMethod<decltype(l)>(func,l));}
#define APPENDMETHOD(metatbl,func,lambda)   { auto l = lambda; metatbl->append_method(func, new NativeMethod<decltype(l)>(func,l));}
#else
//The version of this define for the good boys and girls who actually fucking comply with standard
#define NATIVEMETHOD(objtype,func,lambda) objtype->set_typemethod_raw(func, new NativeMethod(func,lambda));
#define APPENDMETHOD(metatbl,func,lambda) metatbl->append_method(func, new NativeMethod(func,lambda));
#endif