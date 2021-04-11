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

#define PROGRAM 2

int main()
{



#if PROGRAM == 1
	/*
	main()
	{
		return 2 + 1;
	}
	*/
	Function main = Function("main", &ReturnStatement(&BinaryExpression(
		BinaryExpression::bOps::Add,
		&Literal(Value(2)),
		&Literal(Value(1))
	)));
#elif PROGRAM == 2
	/*
	main()
	{
		a = 7;
		b = 3;
		return a + b;
	}
	*/

	Function main = Function("main", &AssignmentStatement(&Identifier("a"), &Literal(Value(7))));
	main.append(&AssignmentStatement(&Identifier("b"), &Literal(Value(3))));
	main.append(&ReturnStatement(&BinaryExpression(
		BinaryExpression::bOps::Add,
		&Identifier("a"),
		&Identifier("b")
	)));
#endif
	Program parsed = Program(&main);

	Interpreter interpreter;
	parsed.set_interp(interpreter);

	Value v = interpreter.execute(parsed);

	std::cout << v.t_value.as_int;


	return EXIT_SUCCESS;
}