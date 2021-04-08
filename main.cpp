/*
GOAL:

Create an interpreter which can run "Rake" or "João," a simple OOP language made to use a directory-oriented object tree

Somewhat dynamically typed but lets not get too angsty about it


*/
#include <iostream>

#include "Forward.h"
#include "AST.h"
#include "Interpreter.h"
#include "Object.h"
#include "Parser.h"

int main()
{
	//std::vector<int> foo = std::vector<int>({ 1,3,4 });

	Literal a = Literal(Value(2)), b = Literal(Value(1));
	BinaryExpression binexpr = BinaryExpression(
		BinaryExpression::bOps::Add,
		a,
		b
	);
	ReturnStatement ret = ReturnStatement(binexpr);
	std::vector<Expression> vret = { ret };

	Function foo = Function("main", vret);

	Program parsed = Program(foo);

	Interpreter interpreter;
	parsed.set_interp(interpreter);

	Value v = interpreter.execute(parsed);

	std::cout << v.t_value.as_int;


	return EXIT_SUCCESS;
}