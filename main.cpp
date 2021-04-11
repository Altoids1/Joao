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

	Identifier a = Identifier("a");
	Identifier b = Identifier("b");
	Literal seven = Literal(Value(7));
	Literal three = Literal(Value(3));
	AssignmentStatement a_seven = AssignmentStatement(&a, &seven);
	AssignmentStatement b_three = AssignmentStatement(&b, &three);
	BinaryExpression a_plus_b = BinaryExpression(
		BinaryExpression::bOps::Add,
		&a,
		&b
	);
	ReturnStatement ret_aplusb = ReturnStatement(&a_plus_b);


	Function main = Function("main", &a_seven);
	main.append(&b_three);
	main.append(&ret_aplusb);
#endif
	Program parsed = Program(&main);

	Interpreter interpreter;
	parsed.set_interp(interpreter);

	Value v = interpreter.execute(parsed);

	std::cout << v.to_string();


	return EXIT_SUCCESS;
}