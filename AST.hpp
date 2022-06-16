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

#if defined(__GNUC__) && __GNUC__ <= 8
//This macro exists, in part, to get around a bug in GCC 8.1,
//that caused it to be unable to do type deduction of lambda expressions for template-class constructors.
//That whole jumble of words is just too jumbly for it to understand, despite it definitely being compliant with C++17 spec.

#define NATIVEMETHOD(objtype,func,lambda) { auto l = lambda; objtype->set_typemethod_raw(func, new NativeMethod<decltype(l)>(func,l));}
#define APPENDMETHOD(metatbl,func,lambda)   { auto l = lambda; metatbl->append_method(func, new NativeMethod<decltype(l)>(func,l));}
//NOTE: NativeFunction also needs these macros in order to instantiate, and so this is an incomplete patch.
//However, by the time I got this far, I had figured out how to avoid 8.1 altogether, mashallah.
//This dead code shall remain as a testament to my triumph and a clue in case this (or a similar) problem comes up again.
#error "GCC 8 and below have bugs in their template type deduction that make compilation impossible. Please use a newer version or a different compiler."
#else
//The version of this define for the good boys and girls who actually fucking comply with standard
#define NATIVEMETHOD(objtype,func,lambda) objtype->append_native_method(func,new NativeMethod(func,lambda));
#define APPENDMETHOD(metatbl,func,lambda) metatbl->append_method(func,new NativeMethod(func,lambda));
#endif