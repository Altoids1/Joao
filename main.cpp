/*
GOAL:

Create an interpreter which can run "Rake" or "João," a simple OOP language made to use a directory-oriented object tree

Somewhat dynamically typed but lets not get too angsty about it


*/
#include <iostream>

#include "Forward.h"
#include "AST.h"
#include "Parser.h"
#include "Interpreter.h"
#include "Object.h"

int main()
{
	Program parsed(Function("main", std::vector<Expression>{BinaryExpression(
		
	)});

	Interpreter interpreter;
	parsed.set_interp(interpreter);

	Value v = interpreter.execute(parsed);

	std::cout << v.t_value.as_double;


	return EXIT_SUCCESS;
}