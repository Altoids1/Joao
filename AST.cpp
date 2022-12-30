#include "Forward.h"
#include "AST.h"
#include "Object.h"
#include "Table.h"
#include "Interpreter.h"
#include "Parser.h"
#include "FailureOr.h"


#define UN_ENUMS(a,b) ((uint32_t(a) << 8)  | uint32_t(b) )


#ifdef JOAO_SAFE
int Expression::expr_count = 0;
#endif

Hashtable<const char*, size_t> ImmutableString::cstr_to_refcount;

Value ASTNode::resolve(Interpreter& interp)
{
	interp.RuntimeError(this, "Attempted to resolve() an abstract ASTNode! (Type:" + class_name()  + ")");
	return Value();
}

Value ASTNode::const_resolve(Parser& parse, bool loudfail)
{
	if (loudfail)
	{
		parse.ParserError(nullptr, "Attempted to const_resolve() an abstract ASTNode! (Type:" + class_name() + ")");
	}
	return Value();
}

Value& ASTNode::handle(Interpreter& interp)
{
	interp.RuntimeError(this, "Attempted to turn " + class_name() + " into a variable handle!");
	return Value::dev_null;
}


//resolve()
Value Literal::resolve(Interpreter& interp)
{
	return heldval;
}
Value Literal::const_resolve(Parser& parse, bool loudfail)
{
	return heldval;
}

Value Identifier::resolve(Interpreter& interp)
{
	// So to "resolve" this means to get its current value, if it has one.
	//Some uses of this class may actually not call the resolve function, and instead simply use it to store the string name of a variable,
	//so as to be able to set it within a scope or, whatever, like for assignment and assignment-y purposes.

	return interp.get_var(t_name,this);
}

Value& Identifier::handle(Interpreter& interp)
{
	//We actually have to evaluate here whether or not the identifier present is pointing to a function or not, at the current scope.

	Function* funky = interp.get_func(t_name.to_string(),this,false);
	if (funky)
	{
		return funky->to_value();
	}
	else
	{
		return interp.get_var(t_name, this);
	}
}



Value AssignmentStatement::resolve(Interpreter& interp)
{
#ifdef JOAO_SAFE
	increment();
#endif
	Value rhs_val = rhs->resolve(interp);

	Value& lhs_val = id->handle(interp);

	if (lhs_val.t_vType == Value::vType::Function)
	{
#if !defined(JOAO_SAFE) && defined(DEBUG)
		std::cout << "WARNING: Overwriting a Value which stores a function pointer!\n";
#else
		return Value();
#endif
	}

	switch (t_op) // FIXME: Make this suck less
	{
	case(aOps::AssignAdd):
		lhs_val = BinaryExpression::BinaryOperation(lhs_val, rhs_val, BinaryExpression::bOps::Add).get_or_throw(interp);
		break;
	case(aOps::AssignSubtract):
		lhs_val = BinaryExpression::BinaryOperation(lhs_val, rhs_val, BinaryExpression::bOps::Subtract).get_or_throw(interp);
		break;
	case(aOps::AssignMultiply):
		lhs_val = BinaryExpression::BinaryOperation(lhs_val, rhs_val, BinaryExpression::bOps::Multiply).get_or_throw(interp);
		break;
	case(aOps::AssignDivide):
		lhs_val = BinaryExpression::BinaryOperation(lhs_val, rhs_val, BinaryExpression::bOps::Divide).get_or_throw(interp);
		break;
	//TODO: Permit other sorts of ops to be mixed with assignment
	case(aOps::Assign):
		lhs_val = rhs_val;
		break;
	default: UNLIKELY
		interp.RuntimeError(this,"Invalid Assignment statement discovered. This is a bug!");
	}
	return lhs_val; // If anything.
}

Value LocalAssignmentStatement::resolve(Interpreter& interp)
{
#ifdef JOAO_SAFE
	increment();
#endif
	Value rhs_val = rhs->resolve(interp);
	//std::cout << "Their name is " + id->get_str() + " and their value is " + std::to_string(rhs_val.t_value.as_int) + "\n";

	if (t_op != aOps::Assign) UNLIKELY // This is mostly derelict code, but might come up later if composite assignment gets refactored at the AST layer, for some reason.
	{
		interp.RuntimeError(this, "Attempt to call unimplemented Assignment operation: " + std::to_string((int)t_op));
		return rhs_val; // If anything.
	}

	if (!typecheck(rhs_val))
	{
		interp.RuntimeError(this, ErrorCode::FailedTypecheck, "LocalAssignmentStatement failed typecheck!");
		return rhs_val; //FIXME: I'm not sure whether or not I want this runtime to cause the assignment to completely fail like this, or not.
	}
	interp.init_var(static_cast<Identifier*>(id)->get_str(), rhs_val, this);

	return rhs_val;
}
Value LocalAssignmentStatement::const_resolve(Parser& parser, bool loudfail)
{
	return rhs->const_resolve(parser, loudfail);
}

Value UnaryExpression::resolve(Interpreter& interp)
{
#ifdef JOAO_SAFE
	increment();
#endif
	Value rhs = t_rhs->resolve(interp);
	if (interp.error) // If we seemed to have caused an error
	{
		return Value(); // Propagate it up the stack!
	}
	Value::vType rtype = rhs.t_vType;


	uint32_t switcher = UN_ENUMS(t_op, rtype);

	switch (switcher)
	{
	//NEGATE
	case(UN_ENUMS(uOps::Negate,Value::vType::Integer)):
		return Value(-rhs.t_value.as_int);
	case(UN_ENUMS(uOps::Negate, Value::vType::Double)):
		return Value(-rhs.t_value.as_double);
	//NOT
	case(UN_ENUMS(uOps::Not, Value::vType::Null)): // !NULL === NULL
		return Value();
	case(UN_ENUMS(uOps::Not, Value::vType::Bool)):
		return Value(!rhs.t_value.as_bool);
	case(UN_ENUMS(uOps::Not, Value::vType::Integer)):
		return Value(!rhs.t_value.as_int);
	case(UN_ENUMS(uOps::Not, Value::vType::Double)):
		return Value(!rhs.t_value.as_double);
	case(UN_ENUMS(uOps::Not, Value::vType::String)):
	case(UN_ENUMS(uOps::Not, Value::vType::Object)):
		return Value(false);
	//BITWISENOT
	case(UN_ENUMS(uOps::BitwiseNot, Value::vType::Integer)):
		return Value(~rhs.t_value.as_int);
	case(UN_ENUMS(uOps::BitwiseNot, Value::vType::Double)): // Does, indeed, flip the bits of the double
	{
		double beta = rhs.t_value.as_double;
		unsigned char* charlie = reinterpret_cast<unsigned char*>(&beta);
		for(int i = 0; i < sizeof(double);++i)
		{
			charlie[i] = ~charlie[i];
		}
		return Value(beta);
	}
	//LENGTH
	case(UN_ENUMS(uOps::Length, Value::vType::String)):
		return Value(rhs.t_value.as_string_ptr->size());
	case(UN_ENUMS(uOps::Length, Value::vType::Object)):
		if (rhs.t_value.as_object_ptr->is_table())
			return Value(static_cast<Table*>(rhs.t_value.as_object_ptr)->length());
	//Rolls over into the default error case on length failure
	[[fallthrough]];
	default:
		interp.RuntimeError(this, ErrorCode::FailedOperation, "Failed to do an Unary operation! (" + rhs.to_string() + ")\nType: (" + rhs.typestring() + ")");
		return Value();
	}
}

std::pair<ImmutableString, Value> LocalAssignmentStatement::resolve_property(Parser& parser)
{
	//step 1. get string
	const ImmutableString& suh = static_cast<Identifier*>(id)->get_str();

	//step 2. get value
	Value ruh = rhs->const_resolve(parser, true);

	if(ty == LocalType::Local)
		parser.ParserError(nullptr, "Unknown or underimplemented LocalType enum detected!");

	if(!typecheck(ruh))
		parser.ParserError(nullptr, "Const-resolved rvalue for ObjectType property failed typecheck!");

	//step 3. profit!
	return std::pair<ImmutableString, Value>(suh,ruh);
}

Value BinaryExpression::resolve(Interpreter& interp)
{
#ifdef JOAO_SAFE
	increment();
#endif
	//The Chef's ingredients: t_op, t_lhs, t_rhs
	Value lhs = t_lhs->resolve(interp);
	if (interp.error) // If we somehow gained a novel error in the course of resolving the operands
	{
		return Value(); // Propagate that. I'm a BinaryExpression, not a TryCatch.
	}
	Value rhs = t_rhs->resolve(interp); //TODO: This fails the principle of short-circuiting but we'll be fine for now
	if (interp.error) // Ditto.
	{
		return Value();
	}
	return BinaryOperation(lhs, rhs, t_op).get_or_throw(interp);
}




void Function::give_args(Interpreter& interp, std::vector<Value>& args, Object* o = nullptr)
{
	if (o)
	{
		obj = o;
	}
	t_args = std::move(args);
	if (t_args.size() < t_argnames.size()) // If we were not given enough arguments
	{
		for (size_t i = t_args.size(); i < t_argnames.size(); ++i)
		{
			t_args.push_back(Value());
		}
	}
}
Value Function::resolve(Interpreter & interp)
{
	interp.push_stack(Directory::lastword(t_name),obj);

	for (int i = 0; i < t_args.size() && i < t_argnames.size(); ++i)
	{
		interp.init_var(t_argnames[i], t_args[i], this);
	}
	t_args = {}; // Reset args
	for (auto it = statements.begin(); it != statements.end(); ++it)
	{
		//it is a pointer to a pointer to an Expression.
		Expression* ptr = *it;
		Value vuh = ptr->resolve(interp); // I dunno.
		if (interp.FORCE_RETURN)
		{
			interp.FORCE_RETURN = false;
			interp.pop_stack();
			obj = nullptr; // Reset object
			if (interp.BREAK_COUNTER) UNLIKELY
			{
				interp.RuntimeError(this, ErrorCode::BadBreak, "Break statement integer too large!");
				interp.BREAK_COUNTER = 0;
			}
			if(interp.CONTINUE_FLAG) UNLIKELY // TODO: Detect this at compiletime
			{
				interp.RuntimeError(this, ErrorCode::BadBreak, "Unexpected continue in function block!");
				interp.CONTINUE_FLAG = false; // consume it anyways
			}
			return vuh;
		}
		if(interp.error) // Oh no, an uncaught runtime!
		{
			interp.pop_stack();
			interp.BREAK_COUNTER = 0;
			interp.CONTINUE_FLAG = false;
			obj = nullptr; // Reset object
			//TODO: Allow for die-as-null functions in contexts where we are not in a try-catch'd call stack.
			//interp.UncaughtRuntime(interp.error);
			return vuh;
		}
	}
	interp.pop_stack();
	obj = nullptr; // Reset object
	if(interp.BREAK_COUNTER) UNLIKELY
	{
		interp.RuntimeError(this, ErrorCode::BadBreak, "Break statement integer too large!");
		interp.BREAK_COUNTER = 0;
	}
	if(interp.CONTINUE_FLAG) UNLIKELY
	{
		interp.RuntimeError(this, ErrorCode::BadBreak, "Unexpected continue in function block!");
		interp.CONTINUE_FLAG = false; // consume it anyways
	}
	return returnValue;
}

Value NativeMethod::resolve(Interpreter& interp)
{
	if (!obj && !is_static)
		interp.RuntimeError(this, "Cannot call NativeMethod without an Object!");

	Value result = lambda(t_args, obj);
	if (result.t_vType == Value::vType::Null && result.t_value.as_int)
	{
		switch (result.t_value.as_int)
		{
		case(static_cast<Value::JoaoInt>(ErrorCode::NoError)): // An expected null, function returned successfully.
			break;
		case(static_cast<Value::JoaoInt>(ErrorCode::BadArgType)):
			interp.RuntimeError(this, ErrorCode::BadArgType, "Args of improper type given to NativeMethod!");
			break;
		case(static_cast<Value::JoaoInt>(ErrorCode::NotEnoughArgs)):
			interp.RuntimeError(this, ErrorCode::NotEnoughArgs, "Not enough args provided to NativeMethod!");
			break;
		default:
			interp.RuntimeError(this, "Unknown RuntimeError in NativeMethod!");
		}
	}
	return result; // Woag.
}

Value ReturnStatement::resolve(Interpreter& interp)
{
	//First get the value
	Value ret;
	if (has_expr)
		ret = held_expr->resolve(interp);
	//THEN!! flag the return
	interp.FORCE_RETURN = true;
	return ret;
}

Value CallExpression::resolve(Interpreter& interp)
{
#ifdef JOAO_SAFE
	increment();
#endif
	Value func = func_expr->handle(interp);
	if (func.t_vType != Value::vType::Function)
	{
		interp.RuntimeError(this, ErrorCode::BadCall, "Attempted to call '" + func.to_string() + "' which is not a function nor method!");
		return Value();
	}

	Function* fptr = func.t_value.as_function_ptr;
	std::vector<Value> vargs;
	for (auto it = args.begin(); it != args.end(); ++it)
	{
		ASTNode* e = *it;
		vargs.push_back(e->resolve(interp));
		if(interp.error) // If there's a novel error from building the arguments, we should just get out.
			return Value();
	}

	Object* obj = fptr->get_obj();
	if (obj)
	{
		return obj->call_method(interp, Directory::lastword(fptr->get_name()), vargs);
	}

	fptr->give_args(interp, vargs);
	return fptr->resolve(interp);
}

//HOPEFULLY! this is an rvalue-kinda situation we have on our hands here
Value& CallExpression::handle(Interpreter& interp)
{
	interp.tempvalue = resolve(interp);
	return interp.tempvalue;
}



Value Block::iterate(const std::vector<Expression*>& state, Interpreter& interp)
{
	for(Expression* ptr : state)
	{
		Value vuh = ptr->resolve(interp); // I dunno.
		if (interp.FORCE_RETURN || interp.error || interp.BREAK_COUNTER || interp.CONTINUE_FLAG)
		{
			return vuh;
		}
	}
	return Value();
}

Value Block::iterate_statements(Interpreter& interp)
{
	return iterate(statements,interp);
}

Value IfBlock::resolve(Interpreter& interp)
{
#ifdef JOAO_SAFE
	increment();
#endif
	if (condition && !condition->resolve(interp)) // If our condition exists (so we're not an Else) and fails
	{
		if (Elseif) // If we have an Elseif to point to
			return Elseif->resolve(interp); // Return that
		return Value(); // Otherwise return Null, I guess.
	}
	//Getting here either means our condition is true or we have no condition because we're an else statement
	//Either way, lets go innit

	interp.push_block("if");
	Value blockret = iterate_statements(interp);
	interp.pop_block();
	return blockret;
}

Value ForBlock::resolve(Interpreter& interp)
{
#ifdef JOAO_SAFE
	Expression::increment();
#endif
	interp.push_block("for"); // Has to happen here since initializer takes place in the for-loops var stack
	if (initializer)
		initializer->resolve(interp);
	while (condition && condition->resolve(interp))
	{
		interp.push_block("for2"); // The "secondary layer" of the for-loop's block, which is actually reset each iteration
		Value blockret = iterate_statements(interp);
		interp.pop_block();
		if (interp.BREAK_COUNTER)
		{
			interp.BREAK_COUNTER -= 1; // I don't trust the decrement operator with this and neither should you.
			interp.pop_block();
			return blockret;
		}
		if(interp.CONTINUE_FLAG)
		{
			interp.CONTINUE_FLAG = false; // Consume the continue
		}
		if (interp.FORCE_RETURN || interp.error)
		{
			interp.pop_block();
			return blockret;
		}
		increment->resolve(interp);
	}
	interp.pop_block();
	return Value();
}

Value ForEachBlock::resolve(Interpreter& interp)
{
#ifdef JOAO_SAFE
	Expression::increment();
#endif
	Value tblval = table_node->resolve(interp);
	if (tblval.t_vType != Value::vType::Object || !tblval.t_value.as_object_ptr->is_table())
	{
		interp.RuntimeError(this, ErrorCode::FailedTypecheck, "for-each iterator must be an Object which inherits from /table!");
		return Value();
	}

	Table* tbl = static_cast<Table*>(tblval.t_value.as_object_ptr);
	/*
	Iterates over the elements of the /table given.
	It first attempts to go in numerical order along the keys, starting at 0 and going up until the associated numerical key cannot be found.
	At that point, it iterates over all remaining keys in an unspecified order until all elements have been iterated over.
	*/
	interp.push_block();
	interp.init_var(key_name, Value(), this);
	interp.init_var(value_name, Value(), this);
	//Normal array keys
	size_t array_it = 0;
	for (;array_it < tbl->length(); array_it++)
	{
		interp.override_var(key_name, Value(array_it), this);
		interp.override_var(value_name,tbl->t_array.at(array_it) , this); // FIXME: Maybe it'd be better if this used a Value& instead of copying it? :thinking:
		Value blockret = iterate_statements(interp);
		if (interp.BREAK_COUNTER)
		{
			interp.BREAK_COUNTER -= 1; // I don't trust the decrement operator with this and neither should you.
			interp.pop_block();
			return blockret;
		}
		if(interp.CONTINUE_FLAG)
		{
			interp.CONTINUE_FLAG = false; // Consume the continue
		}
		if (interp.FORCE_RETURN || interp.error)
		{
			interp.pop_block();
			return blockret;
		}
	}
	//Silly keys, still attempting to go incrementally
	//Note that array_it is already at a value that is 1 more than the array portion's length
	//due to the nuance of how for loops are evaluated :)
	for (;;array_it++)
	{
		Value v = tbl->at(interp, Value(array_it));
		if (v.t_vType == Value::vType::Null)
			break;
		interp.override_var(key_name, Value(array_it), this);
		interp.override_var(value_name, v, this);
		Value blockret = iterate_statements(interp);
		if (interp.BREAK_COUNTER)
		{
			interp.BREAK_COUNTER -= 1; // I don't trust the decrement operator with this and neither should you.
			interp.pop_block();
			return blockret;
		}
		if(interp.CONTINUE_FLAG)
		{
			interp.CONTINUE_FLAG = false; // Consume the continue
		}
		if (interp.FORCE_RETURN || interp.error)
		{
			interp.pop_block();
			return blockret;
		}
	}
	//Normal hash keys
	for (auto it = tbl->t_hash.begin(); it != tbl->t_hash.end(); ++it)
	{
		const Value& key = it.key();
		if (key.t_vType == Value::vType::Integer && key.t_value.as_int < array_it) // Skipping over "silly keys" that we've already iterated over
			continue;

		interp.override_var(key_name, key, this);
		interp.override_var(value_name, it.value(), this);
		Value blockret = iterate_statements(interp);
		if (interp.BREAK_COUNTER)
		{
			interp.BREAK_COUNTER -= 1; // I don't trust the decrement operator with this and neither should you.
			interp.pop_block();
			return blockret;
		}
		if(interp.CONTINUE_FLAG)
		{
			interp.CONTINUE_FLAG = false; // Consume the continue
		}
		if (interp.FORCE_RETURN || interp.error)
		{
			interp.pop_block();
			return blockret;
		}
	}
	interp.pop_block();
	return Value();
}

Value WhileBlock::resolve(Interpreter& interp)
{
#ifdef JOAO_SAFE
	increment();
#endif
	interp.push_block("while");
	if(!condition) // if condition not defined
		interp.RuntimeError(this, "Missing condition in WhileBlock!");

	while (condition->resolve(interp))
	{
		interp.push_block("while2");
		Value blockret = iterate_statements(interp);
		interp.pop_block();
		if (interp.BREAK_COUNTER)
		{
			interp.BREAK_COUNTER -= 1;
			interp.pop_block();
			return blockret;
		}
		if (interp.CONTINUE_FLAG)
		{
			interp.CONTINUE_FLAG = false; // Consume the continue
			continue; // lol
		}
		if (interp.FORCE_RETURN || interp.error)
		{
			//std::cout << "Whileloop got to FORCE_RETURN!\n";
			interp.pop_block();
			return blockret;
		}
	}
	interp.pop_block();
	return Value();
}

Value BreakStatement::resolve(Interpreter& interp)
{
#ifdef JOAO_SAFE
	increment();
#endif
	interp.BREAK_COUNTER = breaknum;
	return Value();
}

Value ContinueStatement::resolve(Interpreter& interp)
{
#ifdef JOAO_SAFE
	increment();
#endif
	interp.CONTINUE_FLAG = true;
	return Value();
}

Value MemberAccess::resolve(Interpreter& interp)
{
	Value& fr = front->handle(interp);

	if (fr.t_vType != Value::vType::Object)
	{
		interp.RuntimeError(this, ErrorCode::BadMemberAccess, "Attempted to do MemberAccess on non-Object Value!");
		return Value();
	}
	if (back->class_name() == "Identifier") // This should pretty much always be the case.
	{
		Object*& ptr = fr.t_value.as_object_ptr;
		Value* v = ptr->has_property(interp, static_cast<Identifier*>(back)->get_str());
		if (v) return *v;
		Function* f = ptr->has_method(interp, static_cast<Identifier*>(back)->get_str());
		if (f) return Value(f);
		if (ptr->is_table())
		{
			return static_cast<Table*>(ptr)->at(interp, static_cast<Identifier*>(back)->get_str().to_string());
		}
		interp.RuntimeError(this, ErrorCode::BadMemberAccess, "MemberAccess failed because member does not exist!");
		return Value();
	}

	// This is something confusing, then
	interp.RuntimeError(this, "Unimplemented MemberAccess with second operand of type " + back->class_name() + "!");
	return Value();
}

Value& MemberAccess::handle(Interpreter& interp) // Tons of similar code to MemberAccess::resolve(). Could be merged somehow?
{
	Value& fr = front->handle(interp);
	if (fr.t_vType != Value::vType::Object)
	{
		interp.RuntimeError(this, ErrorCode::BadMemberAccess, "Attempted to do MemberAccess on non-Object Value!");
		return Value::dev_null;
	}
	if (back->class_name() == "Identifier")
	{
		Value* v = fr.t_value.as_object_ptr->has_property(interp, static_cast<Identifier*>(back)->get_str());
		if (v) return *v;
		Function* meth = fr.t_value.as_object_ptr->has_method(interp, static_cast<Identifier*>(back)->get_str());
		if (!meth)
		{
			interp.RuntimeError(this, ErrorCode::BadMemberAccess, "MemberAccess failed because member does not exist!");
			return Value::dev_null;
		}
		meth->set_obj(fr.t_value.as_object_ptr);
		return meth->to_value();
	}

	// This is something confusing, then
	interp.RuntimeError(this, "Unimplemented MemberAccess with second operand of type " + back->class_name() + "!");
	return Value::dev_null;
}

Hashtable<ImmutableString, Value> ClassDefinition::resolve_properties(Parser& parse)
{
	Hashtable<ImmutableString, Value> svluh;

	for (auto it = statements.begin(); it != statements.end(); ++it)
	{
		LocalAssignmentStatement* lassy = *it;

		svluh.insert(lassy->resolve_property(parse));
	}

	return svluh;
}

void ClassDefinition::append_properties(Parser& parse, ObjectType* objtype)
{

	for (auto it = statements.begin(); it != statements.end(); ++it)
	{
		LocalAssignmentStatement* lassy = *it;

		std::pair<ImmutableString, Value> pear = lassy->resolve_property(parse);

		objtype->set_typeproperty(parse, pear.first,pear.second);	
	}
}

Value Construction::resolve(Interpreter& interp)
{
	return interp.makeObject(type.to_string(),args,this); // FIXME: Should not be to_string-ing here >:/
}

Value ParentAccess::resolve(Interpreter& interp)
{
	return interp.get_property(prop,this);
}

Value& ParentAccess::handle(Interpreter& interp)
{
	Object* o = interp.get_objectscope();
	if (!o)
	{
		interp.RuntimeError(this, ErrorCode::BadMemberAccess, "Cannot do ParentAccess in classless function!");
		return Value::dev_null;
	}
	Function* funk = o->has_method(interp,prop.to_string());
	if(funk)
		return funk->to_value();
	return *(o->has_property(interp,prop));
}

Value GrandparentAccess::resolve(Interpreter& interp)
{
	return interp.grand_property(depth, prop, this);
}
Value& GrandparentAccess::handle(Interpreter& interp)
{
/*
	So we might be a function or a property.
	Lets find out!
*/
	return interp.grand_handle(depth, prop, this);
}


Value GlobalAccess::resolve(Interpreter& interp)
{
	return interp.get_global(var, this);
}

Value& GlobalAccess::handle(Interpreter& interp)
{
	Function* funk = interp.get_func(var.to_string(),this,false);
	if(funk)
		return funk->to_value();
	return *interp.has_global(var, this);
}



Value IndexAccess::resolve(Interpreter& interp)
{
	Value& lhs = container->handle(interp);
	Value rhs = index->resolve(interp);

	switch (lhs.t_vType)
	{
	default:
		interp.RuntimeError(this, ErrorCode::BadMemberAccess, "Attempted to index a Value of type " + lhs.typestring() + "!");
		return Value();
	case(Value::vType::String):
		if (rhs.t_vType != Value::vType::Integer)
		{
			interp.RuntimeError(this, ErrorCode::BadMemberAccess, "Cannot index into a String with a Value of type " + rhs.typestring() + "!");
			return Value();
		}

		return Value(std::string({ lhs.t_value.as_string_ptr->at(rhs.t_value.as_int) })); // It's lines like these that remind me why I want to make my own programming language in the first place. Happy 1000th line, by the way!
	case(Value::vType::Object):
	{
		Object* obj = lhs.t_value.as_object_ptr;
		if (obj->is_table())
		{
			Table* la_table = static_cast<Table*>(obj);
			return la_table->at(interp, rhs);
		}

		if (rhs.t_vType != Value::vType::String)
		{
			interp.RuntimeError(this, ErrorCode::BadMemberAccess, "Cannot index into an Object with a Value of type " + rhs.typestring() + "!");
			return Value();
		}

		return obj->get_property(interp, *rhs.t_value.as_string_ptr);
	}
	}
}

Value& IndexAccess::handle(Interpreter& interp)
{
	Value& lhs = container->handle(interp);
	Value rhs = index->resolve(interp);

	if (lhs.t_vType != Value::vType::Object) // FIXME: Allow for index-based setting of string data
	{
		if (lhs.t_vType == Value::vType::String)
		{
			interp.RuntimeError(this, ErrorCode::BadMemberAccess, "Cannot use string indices to set the values of specific characters!"); // TODO: You probably ought to be able to do this
			return Value::dev_null;
		}
		else
		{
			interp.RuntimeError(this, ErrorCode::BadMemberAccess, "Attempted to index a Value of type " + lhs.typestring() + "!");
			return Value::dev_null;
		}
		
	}

	Object* obj = lhs.t_value.as_object_ptr;
	if (!obj->is_table())
		return MemberAccess(container, index).handle(interp); //FIXME: This is so fucked up but it makes so much sense; main issue is that runtimes will report themselves strangely
	//So it's a table then
	return static_cast<Table*>(obj)->at_ref(interp, rhs);
}

Value TryBlock::resolve(Interpreter& interp)
{
#ifdef JOAO_SAFE
	increment();
#endif
	interp.push_block("try");
	Value ret = iterate_statements(interp);
	interp.pop_block();
	//We don't really care about any break/continue flags as a try{}catch{}, just the error one
	if(interp.error) // ah, we caught an error, cool
	{
		interp.push_block("catch");
		interp.init_var(err_name,interp.error,this); // init the error parameter
		interp.error = Value();
		Value ret = iterate(catch_statements,interp);
		if(interp.error)
		{
			interp.UncaughtRuntime(interp.error);
		}
		interp.pop_block();
		return ret;
	}
	return ret;

}

Value ThrowStatement::resolve(Interpreter& interp)
{
#ifdef JOAO_SAFE
	increment();
#endif
	if (!err_node)
	{
		interp.RuntimeError(this, ErrorCode::Unknown, "Exception thrown!");
		return Value();
	}

	Value val = err_node->resolve(interp);
	if (interp.error)
		return Value();
	if (!val || val.t_vType != Value::vType::Object || !val.t_value.as_object_ptr->object_type.begins_with("/error"))
	{
		interp.RuntimeError(this, ErrorCode::FailedTypecheck, "Throw keyword invoked with non-error operand!");
		return Value();
	}

	interp.RuntimeError(this, val);

	return Value(); // If anything.
}

Value BaseTableConstruction::resolve(Interpreter& interp)
{
	Hashtable<std::string, Value> resolved_entries;
	resolved_entries.ensure_capacity(nodes.size());
	for (auto it : nodes)
	{
		resolved_entries[it.first] = it.second->resolve(interp);
#ifdef JOAO_SAFE
		++interp.value_init_count;
		if (interp.value_init_count > MAX_VARIABLES)
		{
			throw error::max_variables(std::string("Program reached the limit of ") + std::to_string(MAX_VARIABLES) + std::string("instantiated variables!"));
		}
#endif
	}
	if (interp.error)
	{
		return interp.makeBaseTable();
	}
	return interp.makeBaseTable({}, resolved_entries,this);
}

std::vector<ConstExpression*> ConstExpression::_registry;

Value ConstExpression::const_resolve(Parser& parser, bool should_throw) {
	if(should_throw)
		parser.ParserError(nullptr, "Being able to use 'const' in this context has yet to be implemented. :(");
	return Value();
}

Value ConstExpression::resolve(Interpreter& interp) {
	interp.RuntimeError(nullptr, ErrorCode::Unknown, "ConstExpression was mysteriously evaluated during runtime! Very strange!");
	//Since we mutate 'this' when evaluating ConstExpression, I don't even really feel particulary safe just trying to execute at runtime like normal.
	//Just return null.
	return Value();
}

void ConstExpression::transmute(Interpreter& interp) {
	static_assert(sizeof(Literal) <= sizeof(ConstExpression), "ConstExpression needs to be bigger than Literal for its black magic to work! >:(");
	Value result = iterate_statements(interp);
	this->~ConstExpression(); // DELETE THIS
	new (this) Literal(result); // *yoda deathsound*
	return; // I'M DEAD
}