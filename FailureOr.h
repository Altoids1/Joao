#pragma once
#include "Forward.h"
#include "ImmutableString.h"
#include "SharedEnums.h"
#include "Value.h"

//Used in some contexts to store a runtime when the interpreter maybe not be available
//(or might not exist, as is the case during const-folding)
struct FailureOr
{
    struct Failure
    {
        ErrorCode code;
        ImmutableString what;
    };
    bool didError;
    std::variant<Value, Failure> data;

    FailureOr(Value&& ret)
        :didError(false)
        ,data(ret)
    {
    }
    FailureOr(const ImmutableString& _what)
        :didError(true)
        ,data(Failure{ ErrorCode::Unknown, _what })
    {
    }
    FailureOr(ErrorCode _code, const ImmutableString& _what)
        :didError(true)
    {
        Failure fail = { _code, _what };
        data = std::variant<Value, Failure>(fail);
    }

    Value get_or_throw(Interpreter&);
    Value must_get();
};