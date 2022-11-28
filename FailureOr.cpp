#include "FailureOr.h"
#include "Interpreter.h"
#include "Value.h"

//Tries to get the value stored in this. Returns null and marks a runtime on the interpreter if it fails.
//Currently only defined for the Value specialization.
template<>
Value FailureOr<Value>::get_or_throw(Interpreter& interp)
{
    if(didError)
    {
        interp.RuntimeError(nullptr,data.code, data.what.to_string());
        return Value();
    }
    return data.value;
}