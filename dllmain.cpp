#define DLL_CRAP extern "C" __declspec(dllexport) // There's probably other ways of constructing this but this is what we're doin.
#include "Forward.h"
#include "AST.h"
#include "Interpreter.h"
#include "Object.h"
#include "Parser.h"

#include <sstream>

#ifndef _WIN32
    #error "Joao only supports compiling as a shared library on Windows."
#endif

DLL_CRAP char* version(int n, char *v[]) 
{
   return const_cast<std::string&>(VERSION_STRING).data(); // Hopefully this works?
}

/*
Two+ args:
0: The script, as a c-string
1+: any args, passed as String values to the script

Returns:
C-string of the result, if it was a string. An empty c-str otherwise. Can also be an error message if we had an error.
*/
DLL_CRAP char* execute_script(int n, char *v[]) 
{
    //TODO: I don't really know whether keeping these things around as statics really makes sense but uhhh, here they shall remain
    static char empty_str = '\0';
    static Value ret;
    
    if(!n)
        return &empty_str;
    std::vector<Value> args;
    for(size_t i = 1; i < n; ++i)
    {
        args.push_back(Value(std::string(v[i])));
    }
    Scanner scanner;
    std::stringstream script_txt = std::stringstream(std::string(v[0]));
    try
	{
		scanner.scan(script_txt);
	}
	catch (error::scanner s_err)
	{
		return const_cast<char*>(s_err.what());
	}
    catch(...) // sheesh
    {
        return &empty_str;
    }
    Parser pears(scanner);
    Program parsed;
	try
	{
		parsed = pears.parse();
	}
	catch (error::parser p_err)
	{
		return const_cast<char*>(p_err.what());
	}
    catch(...)
    {
        return &empty_str;
    }
    Interpreter interpreter(parsed,false);
    try
    {
        Value jargs = interpreter.makeBaseTable(args,{},nullptr);
        ret = interpreter.execute(parsed, jargs);
    }
    catch(Value err)
    {
        if(err.t_vType != Value::vType::Object)
            return &empty_str;
        //FIXME: check to make sure this is specifically an /error object
        std::string* what = err.t_value.as_object_ptr->get_property_raw("what").t_value.as_string_ptr;
        return what->data();
    }
    catch(...)
    {
        return &empty_str;
    }
    if(ret.t_vType == Value::vType::String)
    {
        return ret.t_value.as_string_ptr->data();
    }
    return &empty_str;
}

#undef DLL_CRAP