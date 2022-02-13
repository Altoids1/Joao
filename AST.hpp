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