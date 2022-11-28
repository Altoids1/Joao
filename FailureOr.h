#pragma once
#include "Forward.h"
#include "ImmutableString.h"
#include "SharedEnums.h"

//Used in some contexts to store a runtime when the interpreter maybe not be available
//(or might not exist, as is the case during const-folding)
template <typename Type>
struct FailureOr
{
    union Data
    {
        Type value;
        struct {
            ImmutableString what;
            ErrorCode code;
        };
        Data() {}
        ~Data() {} // FailureOr handles it :)
    } data;
    bool didError;

    FailureOr(Type&& ret)
        :didError(false)
    {
        data.value = std::move(ret);
    }
    FailureOr(const ImmutableString& _what)
        :didError(true)
    {
        data.what = _what;
        data.code = 1;
    }
    FailureOr(ErrorCode _code, const ImmutableString& _what)
        :didError(true)
    {
        data.what = _what;
        data.code = _code;
    }

    ~FailureOr()
    {
        if(didError)
            data.what.~ImmutableString();
        else
            data.value.~Type();
    }

    Type get_or_throw(Interpreter&);
};