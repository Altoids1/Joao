#pragma once

#include "AST.h"

class Interpreter
{

public:
	Interpreter();

	Value execute(Program&);
};