#include "Parser.h"

#define SYMBOL_ENUMS(a,b) ((a << 9) | b)



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
		lvalue = new Identifier(readIdentifierStr(true));
		break;
	case(Token::cEnum::EndLineToken):
		ParserError(t, "Endline found when expression was expected!");
		break;
	case(Token::cEnum::SymbolToken):
	{
		//This could be two things: a directory-scoped variable Identifier,
		//or a Unary operator.

		//check for the former
		SymbolToken* st = static_cast<SymbolToken*>(t);
		if (st->get_symbol()[0] == '/' || st->get_symbol()[0] == '.')
		{
			lvalue = new Identifier(readIdentifierStr(false));
			break;
		}

		//Then try the latter
		ParserError(t, "Unary operators are not implemented!");
		break;
	}
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
	/*if (t2->class_enum() == Token::cEnum::EndLineToken)
	{
		if (!expecting_semicolon)
		{
			ParserError(t2, "Unexpected semicolon in expression!");
		}
		return lvalue;
	}
	else */if (t2->class_enum() == Token::cEnum::SymbolToken)
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

std::vector<Expression*> Parser::readBlock(BlockType bt) // Tokenheader state should point to the opening brace of this block.
{
	//Blocks are composed of a starting brace, following by statements, and are ended by an end brace.

	//Starting brace checks
	Token* t = tokens[tokenheader];
	if (t->class_enum() != Token::cEnum::PairSymbolToken)
	{
		ParserError(t, "Unexpected character where open-brace was expected!");
	}
	PairSymbolToken pt = *static_cast<PairSymbolToken*>(t);
	if (!pt.is_start || pt.t_pOp != PairSymbolToken::pairOp::Brace)
	{
		ParserError(t, "Unexpected character where open-brace was expected!");
	}


	//In AST land, blocks are a list of expressions associated with a particular scope.
	std::vector<Expression*> ASTs;
	++tokenheader;
	//I wanna point out that block is the only grammar object that has stats, so we can unroll the description of stats into this for-and-switch.
	for (; tokenheader < tokens.size(); ++tokenheader)
	{
		t = tokens[tokenheader];
		switch (t->class_enum())
		{
			//this switch kinda goes from most obvious implementation to least obvious, heh
		case(Token::cEnum::EndLineToken):
			ParserError(t, "Unexpected semicolon in global scope!"); // Yes I'm *that* picky, piss off
			continue;
		case(Token::cEnum::KeywordToken):
		{
			KeywordToken kt = *static_cast<KeywordToken*>(t);
			switch (kt.t_key)
			{
			case(KeywordToken::Key::For):
				ParserError(t, "For-loops are not implemented!");
				continue;
			case(KeywordToken::Key::If):
			case(KeywordToken::Key::Elseif):
			case(KeywordToken::Key::Else):
				ParserError(t, "If statements are not implemented!");
				continue;
			case(KeywordToken::Key::Break):
				ParserError(t, "Break statements are not implemented!");
				continue;
			case(KeywordToken::Key::While):
				ParserError(t, "While-loops are not implemented!");
				continue;
			case(KeywordToken::Key::Return):
			{
				++tokenheader; // Consume this return token
				ReturnStatement* rs = new ReturnStatement(readExp(tokens[tokenheader], true));
				ASTs.push_back(rs);
				continue;
			}
			default:
				ParserError(t, "Unknown keyword!");
				continue;
			}
		}
		case(Token::cEnum::PairSymbolToken):
		{
			PairSymbolToken pt = *static_cast<PairSymbolToken*>(t);
			if (pt.t_pOp == PairSymbolToken::pairOp::Brace && !pt.is_start)
			{
				//This pretty much has to be the end of the block; lets return our vector of shit.
				++tokenheader;
				goto BLOCK_RETURN_ASTS; // Can't break because we're in a switch in a for-loop :(
			}

		}
		case(Token::cEnum::SymbolToken):
			/*
			If the Grammar serves me right, this is either a varstat or a functioncall.

			VARSTAT:
			there's three ways of doing varstats: in the Local scope, Object Scope, or Global Scope.
			x = 3; -- local (or property!) assignment
			./x = 3; -- assignment of property in object scope (this/x = 3; is also valid and more explicit)
			/x = 3; -- global scope asignment (glob/x = 3 also valid, and more explicit since you're saying you're setting a Value and not just getting a type)
			*/

			ParserError(t, "Non-local scope assignments are not implemented! " + t->class_name());
		case(Token::cEnum::WordToken):
		{
			//CASE 1. local/property assign

			WordToken wt = *static_cast<WordToken*>(t);

			Identifier* id = new Identifier(readIdentifierStr(true));
			++tokenheader;
			AssignmentStatement::aOps aesop = readaOp();

			ASTNode* rvalue = readExp(tokens[tokenheader], false);

			ASTs.push_back(new AssignmentStatement(id, rvalue, aesop));

			continue;
		}
		case(Token::cEnum::StringToken):
		case(Token::cEnum::NumberToken):
			ParserError(t, "Misplaced literal detected when lvalue expected");
			break;
		default:
			ParserError(t, "Unknown Token type found when traversing block!");
		}
	}
BLOCK_RETURN_ASTS:
	if (ASTs.size() == 0)
	{
		ParserError(t, "Block created with no Expressions inside!");
	}

	return ASTs;
}