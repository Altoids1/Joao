#include "FailureOr.h"
#include "Interpreter.h"
#include "Value.h"

//Tries to get the value stored in this. Returns null and marks a runtime on the interpreter if it fails.
//Currently only defined for the Value specialization.
Value FailureOr::get_or_throw(Interpreter& interp)
{
    if(didError)
    {
        if (this->data.index() != 1) {
            interp.RuntimeError(nullptr, ErrorCode::Unknown, "FailureOr failed to store the actual error message!");
            return Value();
        }
        Failure& fail = std::get<Failure>(data);
        interp.RuntimeError(nullptr, fail.code, fail.what.to_string());
        return Value();
    }
    return std::get<Value>(data);
}

Value FailureOr::must_get()
{
    return std::get<Value>(data);
}