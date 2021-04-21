#include "../Program.h"

#include <limits>

#define MATH_E 2.71828182845904523536
#define PI 3.14159265358979323846

#define NATIVE_FUNC(name) definedFunctions[##name] = &NativeFunction(##name, [](std::vector<Value> args)

void Program::construct_math_library()
{
	//MATHEMATICS

}

#undef NATIVE_FUNC
#undef PI
#undef MATH_E