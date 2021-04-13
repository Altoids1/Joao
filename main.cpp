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


#define PROGRAM 3

int main(int argc, char** argv)
{
#if PROGRAM == 1 // Tests adding two literals together and returning
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
#elif PROGRAM == 2 // Tests setting vars and doing math with them
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
#elif PROGRAM == 3 // Tests Values containing strings, and doing math with them
	/*
	main()
	{
		a = "Hello";
		b = " World!";
		return a + b;
	}
	*/
	Identifier a = Identifier("a");
	Identifier b = Identifier("b");
	std::string h = "Hello", _w = " World!";
	Literal hello = Literal(Value(h));
	Literal _world = Literal(Value(_w));
	AssignmentStatement a_seven = AssignmentStatement(&a, &hello);
	AssignmentStatement b_three = AssignmentStatement(&b, &_world);
	BinaryExpression a_plus_b = BinaryExpression(
		BinaryExpression::bOps::Add,
		&a,
		&b
	);
	ReturnStatement ret_aplusb = ReturnStatement(&a_plus_b);


	Function testmain = Function("main", &a_seven);
	testmain.append(&b_three);
	testmain.append(&ret_aplusb);
#endif
	Program parsed;
	if (argc == 1)
	{
		parsed = Program(&testmain);
	}
	else
	{
		std::ifstream file;
		std::cout << "Opening file " << argv[1] << "...\n";
		file.open(argv[1]);
		if (file.bad())
		{
			std::cout << "Unable to open file " << argv[1] << "!\n";
			exit(1);
		}
		Scanner scn;
		scn.scan(file);
		file.close();

	}

	Interpreter interpreter;
	parsed.set_interp(interpreter);

	Value v = interpreter.execute(parsed);

	std::cout << v.to_string();


	return EXIT_SUCCESS;
}