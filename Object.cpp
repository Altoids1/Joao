#include "Object.h"
#include "Interpreter.h"


Value Object::get_property(Interpreter& interp, std::string name)
{
	if (properties.count(name))
		return properties.at(name);
	if (base_properties->count(name))
		return base_properties->at(name);
	interp.RuntimeError(nullptr, "Unable to access property of object!");
	return Value();
}
void Object::set_property(Interpreter& interp, std::string name, Value rhs)
{
	if (base_properties->count(name) == 0)
		interp.RuntimeError(nullptr, "Unable to access property of object!");

	if (properties.count(name))
	{
		properties.erase(name);
		properties[name] = rhs;
	}
	else
	{
		properties[name] = rhs;
	}
}

Value Object::call_method(Interpreter& interp, std::string name, std::vector<Value> &args)
{
	if (!base_funcs->count(name))
		interp.RuntimeError(nullptr, "Unable to access method of object!");

	Function* fuh = base_funcs->at(name);
	fuh->give_args(args, interp);
	return fuh->resolve(interp);
}