#include "Parser.h"

#define SYMBOL_ENUMS(a,b) ((a << 9) | b)



Program Parser::parse() // This Parser is w/o question the hardest part of this to write.
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

				std::vector<Expression*> bluh = readBlock(BlockType::Function,tokenheader, tokens.size()-1);
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
			readBlock(BlockType::Unknown, tokenheader, tokens.size() - 1); // Has to be a function so as to allow itself to call itself recursively.
			--tokenheader;
			continue;
		}
		}

	}

	return t_program;
}

ASTNode* Parser::readlvalue(int here, int there) // Read an Expression where we know for certain that there's no damned binary operator within it.
{
#ifdef LOUD_TOKENHEADER
	std::cout << "readlvalue starting at " << std::to_string(here) << std::endl;
#endif
	Token* t = tokens[here];

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
		tokenheader = here + 1;
#ifdef LOUD_TOKENHEADER
		std::cout << "readlvalue setting tokenheader to " << std::to_string(tokenheader) << std::endl;
#endif
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
		tokenheader = here + 1;
#ifdef LOUD_TOKENHEADER
		std::cout << "readlvalue setting tokenheader to " << std::to_string(tokenheader) << std::endl;
#endif
		break;
	}
	case(Token::cEnum::StringToken):
	{
		StringToken st = *static_cast<StringToken*>(t);
		lvalue = new Literal(Value(st.word));
		tokenheader = here + 1;
#ifdef LOUD_TOKENHEADER
		std::cout << "readlvalue setting tokenheader to " << std::to_string(tokenheader) << std::endl;
#endif
		break;
	}
	case(Token::cEnum::WordToken):
		lvalue = new Identifier(readIdentifierStr(true,here,here));
		break;
	case(Token::cEnum::EndLineToken):
		ParserError(t, "Endline found when lvalue was expected!");
		break;
	case(Token::cEnum::SymbolToken):
	{
		//This could be two things: a directory-scoped variable Identifier,
		//or a Unary operator.

		//check for the former
		SymbolToken* st = static_cast<SymbolToken*>(t);
		if (st->get_symbol()[0] == '/' || st->get_symbol()[0] == '.')
		{
			lvalue = new Identifier(readIdentifierStr(false,here,here));
			break;
		}

		//Then try the latter
		ParserError(t, "Unexpected Unary Operation!");
		break;
	}
	default:
		ParserError(t, "Unexpected Token when reading Expression! " + t->class_name());
		break;
	}

	if (!lvalue)
	{
		ParserError(t, "Failed to comprehend lvalue of Expression!");
	}

	
	return lvalue;
}

// Reads, from here to there, scanning for BinaryExpressions of its OperationPrecedence and lower
ASTNode* Parser::readBinExp(Scanner::OperationPrecedence op, int here, int there, bool expect_close_paren = false) 
{
#ifdef LOUD_TOKENHEADER
	std::cout << "readBinExp(" << Scanner::precedence_tostring(op) << ") starting at " << std::to_string(here) << std::endl;
	if (expect_close_paren)
		std::cout << "I expect close paren!\n";
	else
		std::cout << "I don't expect close paren!\n";
#endif
	//std::cout << "Starting to search for operation " << Scanner::precedence_tostring(op) << "...\n";

	if (op == Scanner::OperationPrecedence::NONE) // If we're at the bottom, evaluate a primary
	{
		Token* t = tokens[here];
		switch (t->class_enum())
		{
		case(Token::cEnum::PairSymbolToken):
			ParserError(t,"Pairing within expressions is not implemented!");
			break;
		default:
			return readlvalue(here,here); // Hear-hear!
		}
	}


	ASTNode* lhs = nullptr;

	int where = here;

	//Now lets try to read the binary operation in question
	for (; where <= there; ++where)
	{
		Token* t2 = tokens[where];
		switch (t2->class_enum())
		{
		case(Token::cEnum::PairSymbolToken):
		{
			PairSymbolToken pst = *static_cast<PairSymbolToken*>(t2);
			if (expect_close_paren && !pst.is_start && pst.t_pOp == PairSymbolToken::pairOp::Paren)
			{ // This is here to handle the (rare) circumstance of a closeparen having the same effect as a semicolon, such as in the third statement of a for-loop construction
				goto READBOP_LEAVE_BOPSEARCH;
			}
			
			ParserError(t2, "Unexpected or unimplemented Pairsymbol in BinaryExpression!");
			continue;
		}
		case(Token::cEnum::EndLineToken):
			//++where; // For... reasons, BinExp must leave pointing to the semicolon it found, if it did find one
			goto READBOP_LEAVE_BOPSEARCH;
		case(Token::cEnum::KeywordToken):
			ParserError(tokens[where], "Unexpected keyword in Expression!");
			continue;
		case(Token::cEnum::SymbolToken):
		{
			SymbolToken st = *static_cast<SymbolToken*>(t2);

			BinaryExpression::bOps boopitybeep;

			if (st.len == 1)
			{
				boopitybeep = readbOpOneChar(st.get_symbol(), t2);
			}
			else
			{
				boopitybeep = readbOpTwoChar(st.get_symbol(), t2);
			}

			if (bOp_to_precedence.at(boopitybeep) == op) // WE GOT A HIT!
			{
				
				//ALL THIS ASSUMES LEFT-ASSOCIATIVITY, AS IN ((1 + 2) + 3) + 4

				if (!lhs)
				{
					lhs = readBinExp(static_cast<Scanner::OperationPrecedence>(static_cast<uint8_t>(op) - 1), here, where-1, expect_close_paren);
				}

				ASTNode* right = readBinExp(static_cast<Scanner::OperationPrecedence>(static_cast<uint8_t>(op) - 1), where+1, there, expect_close_paren);
				
				lhs = new BinaryExpression(boopitybeep, lhs, right);
				
				continue;
			}
			else if (bOp_to_precedence.at(boopitybeep) > op)
			{
				goto READBOP_LEAVE_BOPSEARCH;
			}

			//we don't got a hit. :(
			continue;
		}
		default:
			continue;
		}
	}
READBOP_LEAVE_BOPSEARCH:
	//std::cout << "Exiting search for " << Scanner::precedence_tostring(op) << " at token " << where << "...\n";
#ifdef LOUD_TOKENHEADER
	std::cout << "readBinExp(" << Scanner::precedence_tostring(op) << ") setting tokenheader to " << std::to_string(where) << std::endl;
#endif
	tokenheader = where; // FIXME: I don't even really know what exactly there is to fix here, just know that readBinExp does some funky bullshit with the tokenheader that may cause it to malpoint in anything readBinExp calls
	if (lhs)
		return lhs;

	//ParserError(tokens[here], "readBinExp failed to read binary expression!");


	//If we're here then it seems the operation(s) we're looking for doesn't exist
	//So lets just return... whatever it is in the lower stacks
	lhs = readBinExp(static_cast<Scanner::OperationPrecedence>(static_cast<uint8_t>(op) - 1), here, there, expect_close_paren);


	
	return lhs;
	
}

ASTNode* Parser::readExp(int here, int there, bool expect_close_paren = false)
{
#ifdef LOUD_TOKENHEADER
	std::cout << "Expression starts at " << std::to_string(here) << std::endl;
#endif
	//Okay, god, we're really getting close to actually being able to *resolve* something.

	Token* t = tokens[here];

	Scanner::OperationPrecedence lowop = lowest_ops[t->syntactic_line];

	if (lowop == Scanner::OperationPrecedence::NONE) // If we know for a fact that no binary operation takes place on this syntactic line
	{
		//++tokenheader;
#ifdef LOUD_TOKENHEADER
		ASTNode* ans = readlvalue(here, there);
		std::cout << "Expression leaves at " << std::to_string(tokenheader) << std::endl;
		return ans;
#else
		return readlvalue(here, there);
#endif
	}

	//we know (or at least kinda think) that there's a binop afoot.
#ifdef LOUD_TOKENHEADER
	ASTNode* ans = readBinExp(lowop, here, there, expect_close_paren);
	std::cout << "Expression leaves at " << std::to_string(tokenheader) << std::endl;
	return ans;
#else
	return readBinExp(lowop, here, there, true);
#endif
	
}

LocalAssignmentStatement* Parser::readLocalAssignment(LocalType ty, int here, int there) // Value x = 3;
{
	Token* t = tokens[here];
	if (t->class_enum() != Token::cEnum::LocalTypeToken)
		ParserError(t, "Unexpected Token where LocalTypeToken was expected!");

	switch (static_cast<LocalTypeToken*>(t)->t_type) // I feel like a lvl 10 Wizard when I write lines of C++ like this
	{
	case(LocalType::Value):
		break;
	case(LocalType::Local):
		ParserError(t, "'local' is a reserved word; use 'Local' instead!");
		return nullptr;
	default:
		ParserError(t, "Underimplemented LocalTypeToken detected!");
		return nullptr;
	}

	Identifier* id = new Identifier(readIdentifierStr(true, here+1, here+1));

	AssignmentStatement::aOps aesop = readaOp(here+2);

	ASTNode* rvalue = readExp(here+3,there); // the static_cast here is just to silence dumb compiler warnings

	return new LocalAssignmentStatement(id, rvalue, aesop);
}

std::vector<Expression*> Parser::readBlock(BlockType bt, int here, int there) // Tokenheader state should point to the opening brace of this block.
{
	//Blocks are composed of a starting brace, following by statements, and are ended by an end brace.

	//Starting brace checks
	Token* t = tokens[here];
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
	//++tokenheader;
	//I wanna point out that block is the only grammar object that has stats, so we can unroll the description of stats into this for-and-switch.
	int where = here+1;
	for (; where <= there; ++where)
	{
		t = tokens[where];
		switch (t->class_enum())
		{
			//this switch kinda goes from most obvious implementation to least obvious, heh
		case(Token::cEnum::EndLineToken):
			ParserError(t, "Unexpected semicolon in block!"); // Yes I'm *that* picky, piss off
			continue;
		case(Token::cEnum::KeywordToken):
		{
			KeywordToken kt = *static_cast<KeywordToken*>(t);
			switch (kt.t_key)
			{
			case(KeywordToken::Key::For):
			{
				++where; tokenheader = where;// Move the header past the for keyword
				consume_paren(true); // (
				size_t semicolon = find_first_semicolon(tokenheader, there);
				ASTNode* init = readExp(tokenheader, semicolon);
				where = semicolon + 1;
				semicolon = find_first_semicolon(where, there);
				ASTNode* cond = readExp(where, semicolon);
				where = semicolon + 1;
				ASTNode* inc = readExp(where, there, true);
				consume_paren(false); // )

				std::vector<Expression*> for_block = readBlock(BlockType::For,tokenheader,there); // { ...block... }

				ASTs.push_back(new ForBlock(init, cond, inc, for_block));

				where = tokenheader - 1; // decrement to counteract imminent increment
				

				//ParserError(t, "For-loops are not implemented!");
				continue;
			}
			case(KeywordToken::Key::If):
			{
				++where; tokenheader = where;
				consume_paren(true); // (
				ASTNode* cond = readExp(tokenheader, there, true);
				consume_paren(false); // )

				std::vector<Expression*> if_block = readBlock(BlockType::If,tokenheader,there);

				ASTs.push_back(new IfBlock(cond, if_block));

				where = tokenheader - 1;
				continue;
			}
			case(KeywordToken::Key::Elseif):
			case(KeywordToken::Key::Else):
				ParserError(t, "Elseif & Else statements are not implemented!");
				continue;
			case(KeywordToken::Key::Break):
			{
				if (bt == BlockType::Function)
					ParserError(t, "Unexpected Break statement in Function block!");
				++where; tokenheader = where; // Consume break token
				Token* t2 = tokens[where];

				int brk = 1;

				switch (t2->class_enum())
				{
				case(Token::cEnum::NumberToken):
				{
					NumberToken* nt = static_cast<NumberToken*>(t);
					if(nt->is_double)
						ParserError(t, "Unexpected double literal after Break keyword; 'break' may only take expressionless integer literals as input!");
					brk = nt->num.as_int;
					++where; tokenheader = where; // Consume Number token
					consume_semicolon();
					break;
				}//Rolls over into EndLineToken case
				case(Token::cEnum::EndLineToken):
					consume_semicolon();
					break;
				default:
					ParserError(t, "Unexpected Token after Break keyword; 'break' may only take expressionless integer literals as input!");
					continue;
				}

				ASTs.push_back(new BreakStatement(brk));

				where = tokenheader - 1;
				//ParserError(t, "Break statements are not implemented!");
				continue;
			}
			case(KeywordToken::Key::While):
			{				
				++where; tokenheader = where;
				consume_paren(true); // (
				ASTNode* cond = readExp(tokenheader, there, true);
				consume_paren(false); // )

				std::vector<Expression*> while_block = readBlock(BlockType::While,tokenheader,there);

				ASTs.push_back(new WhileBlock(cond, while_block));

				where = tokenheader - 1;

				//ParserError(t, "While-loops are not implemented!");
				continue;
			}
			case(KeywordToken::Key::Return):
			{
				++where; tokenheader = where; // Consume this return token
				ReturnStatement* rs = new ReturnStatement(readExp(where, there-1));
				ASTs.push_back(rs);
				consume_semicolon();

				where = tokenheader - 1;
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
				tokenheader = where + 1;
				goto BLOCK_RETURN_ASTS; // Can't break because we're in a switch in a for-loop :(
			}

		}
		case(Token::cEnum::SymbolToken):
		{
			/*
			If the Grammar serves me right, this is either a varstat or a functioncall.

			VARSTAT:
			there's four ways of doing varstats: in the Local scope, Object Scope, or Global Scope.
				Value x = 3; ## Set local variable to 3
				/x = 3; ## Set global variable to 3
				./x = 3; ## Set property of object we're in called x to 3
				x = 3; ## Ambiguous, sets lowest-scoped x available to 3
			*/

			SymbolToken st = *static_cast<SymbolToken*>(t);

			if (st.get_symbol()[0] == '/' && st.len == 1)
			{
				ParserError(t, "Non-local scope assignments are not implemented! " + t->class_name());
			}
			else
			{
				ParserError(t, "Unexpected or unimplemented symbol found when traversing Block!");
			}
		}
		case(Token::cEnum::WordToken):
		{
			//CASE 1. local/property assign

			Identifier* id = new Identifier(readIdentifierStr(true,where,where));

			AssignmentStatement::aOps aesop = readaOp();

			
			ASTNode* rvalue = readExp(tokenheader, static_cast<int>(tokens.size()) -1); // the static_cast here is just to silence dumb compiler warnings

			consume_semicolon();

			ASTs.push_back(new AssignmentStatement(id, rvalue, aesop));

			where = tokenheader-1; // Decrement so that the impending increment puts us in the correct place.
			continue;
		}
		case(Token::cEnum::LocalTypeToken): // So this implies we're about to read in an initialization.
		{
			//std::cout << "Before: " << std::to_string(tokenheader) << std::endl ;
			LocalAssignmentStatement* localassign = readLocalAssignment(static_cast<LocalTypeToken*>(t)->t_type, where, there);
			consume_semicolon();
			if(localassign) // if not nullptr
				ASTs.push_back(localassign);
			where = tokenheader - 1; // decrement to counteract imminent increment
			//std::cout << "After: " << std::to_string(tokenheader) << std::endl;
			continue;
		}
		case(Token::cEnum::StringToken):
		case(Token::cEnum::NumberToken):
			ParserError(t, "Misplaced literal detected while traversing block!");
			break;
		default:
			ParserError(t, "Unknown Token type found when traversing block!");
		}
	}
	ParserError(t, "Unable to find ending brace of block!");

BLOCK_RETURN_ASTS:
	if (ASTs.size() == 0)
	{
		ParserError(t, "Block created with no Expressions inside!");
	}

#ifdef LOUD_TOKENHEADER
	std::cout << "Exiting block with header pointed at " << std::to_string(tokenheader) << ".\n";
#endif
	return ASTs;
}