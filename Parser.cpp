#include "Parser.h"

#define SYMBOL_ENUMS(a,b) ((a << 9) | b)

void Program::construct_natives()
{
	//TEXT MANIPULATION
	definedFunctions["print"] = &NativeFunction("print", [](std::vector<Value> args)
	{
		if (args.size())
		{
			std::cout << args[0].to_string();
			for (int i = 1; i < args.size(); ++i)
			{
				std::cout << '\t' << args[i].to_string();
			}
		}
		std::cout << "\n";
		return Value();
	});
	definedFunctions["tostring"] = &NativeFunction("tostring", [](std::vector<Value> args)
	{
		return Value(args[0].to_string());
	});
	definedFunctions["void_stellakafuhparenthessisluaunderscorestatewithacapitalscommaluaunderscoreallocfvoidstarud"] = &NativeFunction("void_stellakafuhparenthessisluaunderscorestatewithacapitalscommaluaunderscoreallocfvoidstarud", [](std::vector<Value> args)
	{
		return Value(7);
	});

	//MATHEMATICS
	definedFunctions["sqrt"] = &NativeFunction("sqrt", [](std::vector<Value> args)
	{
		Value arg = args[0];
		switch (arg.t_vType)
		{
		case(Value::vType::Double):
			return Value(sqrt(arg.t_value.as_double));
		case(Value::vType::Integer):
			return Value(sqrt(arg.t_value.as_int));
		case(Value::vType::Bool):
			return Value(sqrt(arg.t_value.as_bool));
		default:
			return Value(Value::vType::Null, int(ErrorCode::BadArgType));
		}
	});
	definedFunctions["sin"] = &NativeFunction("sin", [](std::vector<Value> args)
	{
		Value arg = args[0];
		switch (arg.t_vType)
		{
		case(Value::vType::Double):
			return Value(sin(arg.t_value.as_double));
		case(Value::vType::Integer):
			return Value(sin(arg.t_value.as_int));
		case(Value::vType::Bool):
			return Value(sin(arg.t_value.as_bool));
		default:
			return Value(Value::vType::Null, int(ErrorCode::BadArgType));
		}
	});
	definedFunctions["cos"] = &NativeFunction("cos", [](std::vector<Value> args)
	{
		Value arg = args[0];
		switch (arg.t_vType)
		{
		case(Value::vType::Double):
			return Value(cos(arg.t_value.as_double));
		case(Value::vType::Integer):
			return Value(cos(arg.t_value.as_int));
		case(Value::vType::Bool):
			return Value(cos(arg.t_value.as_bool));
		default:
			return Value(Value::vType::Null, int(ErrorCode::BadArgType));
		}
	});
}


Program Parser::parse() // This is w/o question the hardest part of this to write.
{
	assert(tokens.size() > 0);

	for (tokenheader = 0; tokenheader < tokens.size(); ++tokenheader)
	{

		switch (grammarstack.front())
		{
		case(GrammarState::program): // This state implies we're surfing the global scope, looking for things to def.
		{
			//Expecting: a bunch of classdefs and funcdefs
			//Both of these start with a directory that ends in a '/'Name, so lets read that in
			std::string dir_name = read_dir_name(); // This, as a side effect, increments tokenheader up to pointing at the first thing that ain't this directory thing.

			//The next token has to be either a '(', which disambiguates us into being a funcdef,
			//or a '{', which brings us towards being a classdef.
			Token* t = tokens[tokenheader];
			if (t->class_enum() != Token::cEnum::PairSymbolToken)
			{
				ParserError(t, "Unexpected Token at global-scope definition!");
				continue; // ?????????
			}

			PairSymbolToken st = *static_cast<PairSymbolToken*>(t); // Allah

			PairSymbolToken::pairOp pop = st.t_pOp;
			if (pop == PairSymbolToken::pairOp::Bracket || !st.is_start)
			{
				ParserError(t, "Unexpected PairSymbol at global-scope definition!");
				continue;
			}
			if (pop == PairSymbolToken::pairOp::Paren) // THIS IS A FUNCDEF! HOT DAMN we're getting somewhere
			{
				//TODO: IMPLEMENT PARAMETER DEFINITIONS
				tokenheader += 2; // jumps over the implied '()', hackish!
				grammarstack.push_front(GrammarState::block);

				std::vector<Expression*> bluh = readBlock(BlockType::Function);
				--tokenheader;

				Function* func = new Function(dir_name, bluh);

				t_program.set_func(dir_name, func);

				continue;
			}
			else if (pop == PairSymbolToken::pairOp::Brace) // THIS IS A CLASSDEF!
			{
				ParserError(t, "Classdefs are not implemented yet!");
			}
			else
			{
				ParserError(t, "Unexpected Pairsymbol at global-scope definition!");
				continue;
			}
		}
		case(GrammarState::block): // This state implies we're entering into a scope
		//Right now, it's the Interpreter's duty to comprehend how things scope out. We're just here to parse things, not run them.
		{
			ParserError(tokens[tokenheader], "Parsing block in an unknown context!");
			readBlock(BlockType::Unknown); // Has to be a function so as to allow itself to call itself recursively.
			--tokenheader;
			continue;
		}
		}

	}

	return t_program;
}

ASTNode* Parser::readExp(Token* t, bool expecting_semicolon = true)
{
	//Okay, god, we're really getting close to actually being able to *resolve* something.

	ASTNode* lvalue = nullptr;


	//First things first, lets find the "lvalue" of this expression, the thing on the left
	switch (t->class_enum())
	{
	case(Token::cEnum::LiteralToken):
	{
		LiteralToken lt = *static_cast<LiteralToken*>(t);
		switch (lt.t_literal)
		{
		case(LiteralToken::Literal::Null):
		{
			lvalue = new Literal(Value());
			break;
		}
		case(LiteralToken::Literal::False):
		{
			lvalue = new Literal(Value(false));
			break;
		}
		case(LiteralToken::Literal::True):
		{
			lvalue = new Literal(Value(true));
			break;
		}
		default:
			ParserError(t, "Unknown LiteralToken type!");
			break;
		}
		break;
	}
	case(Token::cEnum::NumberToken):
	{
		NumberToken nt = *static_cast<NumberToken*>(t);
		if (nt.is_double)
		{
			lvalue = new Literal(Value(nt.num.as_double));
		}
		else
		{
			lvalue = new Literal(Value(nt.num.as_int));
		}
		break;
	}
	case(Token::cEnum::StringToken):
	{
		StringToken st = *static_cast<StringToken*>(t);
		lvalue = new Literal(Value(st.word));
		break;
	}
	case(Token::cEnum::WordToken):
		ParserError(t, "Variables are not implemented!");
		break;
	case(Token::cEnum::EndLineToken):
		ParserError(t, "Endline found when expression was expected!");
		break;
	case(Token::cEnum::SymbolToken):
		//uhh... okay?
		//If this is a unary operator then its fine; lets ask
		ParserError(t, "Unary operators are not implemented!");
		break;
	case(Token::cEnum::PairSymbolToken):
		ParserError(t, "Pairlet operators are not implemented for expressions!");
		break;
	default:
		ParserError(t, "Unexpected Token when reading Expression! " + t->class_name());
		break;
	}

	if (!lvalue)
	{
		ParserError(t, "Failed to comprehend lvalue of Expression!");
	}

	//Now lets see if there's a binary operator and, if so, construct a BinaryExpression() to return.

	++tokenheader;

	Token* t2 = tokens[tokenheader];



	BinaryExpression::bOps bippitybop = BinaryExpression::bOps::NoOp;
	if (t2->class_enum() == Token::cEnum::EndLineToken)
	{
		if (!expecting_semicolon)
		{
			ParserError(t2, "Unexpected semicolon in expression!");
		}
		return lvalue;
	}
	else if (t2->class_enum() == Token::cEnum::SymbolToken)
	{
		SymbolToken st = *static_cast<SymbolToken*>(t2);

		char* c = st.get_symbol();

		switch (st.len)
		{
		case(1):
			bippitybop = readbOpOneChar(c,&st);
			break;
		case(2):
			bippitybop = readbOpTwoChar(c,&st);
			break;
		case(3):
		case(4):
			ParserError(t2, "Binary operations longer than two characters are not implemented!");
			break;
		default:
			ParserError(t2, "SymbolToken with malformed len property detected!");
		}
	}

	if (bippitybop == BinaryExpression::bOps::NoOp) // No operation found, simply return.
	{
		return lvalue;
	}
	++tokenheader;//We're past the binop symbol.
	//We're in "exp binop exp" land now, baby.

	ASTNode* rvalue = readExp(tokens[tokenheader]);
	if (!rvalue)
	{
		ParserError(tokens[tokenheader], "BinaryExpression missing rvalue!");
		//by default have it be.. like, null, I guess.
		rvalue = new Literal(Value());
	}

	return new BinaryExpression(bippitybop, lvalue, rvalue);

	ParserError(t2, "BinaryExpressions are not implemented!");
	return lvalue;
}