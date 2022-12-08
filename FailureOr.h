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
    std::variant<Value, Failure> data;
    bool didError;

    FailureOr(Value&& ret)
        :didError(false)
        ,data(ret)
    {
    }
    FailureOr(const ImmutableString& _what)
        :didError(true)
        ,data(Failure{ .code = ErrorCode::Unknown,.what = _what })
    {
    }
    FailureOr(ErrorCode _code, const ImmutableString& _what)
        :didError(true)
    {
        Failure fail = { .code = _code, .what = _what };
        data = std::variant<Value, Failure>(fail);
    }

    Value get_or_throw(Interpreter&);
};