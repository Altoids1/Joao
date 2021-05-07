#include "../Program.h"
#include "../Object.h"

#define NATIVE_FUNC_TABLE(name) static_cast<Function*>(new NativeFunction( name , [](std::vector<Value> args)


void Program::construct_table_library()
{
	definedObjTypes["/table"] = new ObjectType("/table");
	definedObjTypes["/table"]->is_table_type = true;
	//definedObjTypes["/table"]->set_typemethod_raw("insert",)

}