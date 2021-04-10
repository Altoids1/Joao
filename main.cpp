/*
GOAL:

Create an interpreter which can run "Rake" or "João," a simple OOP language made to use a directory-oriented object tree

Somewhat dynamically typed but lets not get too angsty about it


*/


#include "Forward.h"
#include "AST.h"
#include "Interpreter.h"
#include "Object.h"
#include "Parser.h"


int main()
{

	/*
	main()
	{
		return 2 + 1;
	}
	*/
	Function foo = Function("main", &ReturnStatement(&BinaryExpression(
		BinaryExpression::bOps::Add,
		&Literal(Value(2)),
		&Literal(Value(1))
	)));
	Program parsed = Program(&foo);

	Interpreter interpreter;
	parsed.set_interp(interpreter);

	Value v = interpreter.execute(parsed);

	std::cout << v.t_value.as_int;


	return EXIT_SUCCESS;
}