#pragma once

#include "AST.h"

#include "Scanner.h"
#include "Program.h"

class Parser
{
	Program t_program;
	std::vector<Token*> tokens;
	
	/*
	Here's how this is gonna work:
	The Parser is going to semi-recursively iterate over the various grammar possibilities,
	based on the formal grammar that we have (which is loosely based on how Lua formats it)

	PLEASE preserve the fact that the enums listed here are in the same order as they are defined in JoaoGrammar.txt!
	*/
	enum class GrammarState : uint8_t { // I am become death, the destroyer of stack frames
		program,
		block,
		stat,
		retstat,
		directory,
		classdef,
		funcdef,
		varstat,
		var,
		namelist,
		explist,
		exp,
		prefixexp,
		functioncall,
		args,
		parlist,
		tableconstructor,
		fieldlist,
		field,
		assignop,
		binop,
		unop
	};
	std::list<GrammarState> grammarstack;
	std::vector<Scanner::OperationPrecedence> lowest_ops;

	const std::unordered_map<BinaryExpression::bOps, Scanner::OperationPrecedence> bOp_to_precedence =
	{
		{BinaryExpression::bOps::Add,Scanner::OperationPrecedence::Term},
		{BinaryExpression::bOps::Subtract,Scanner::OperationPrecedence::Term},
		{BinaryExpression::bOps::Multiply,Scanner::OperationPrecedence::Factor},
		{BinaryExpression::bOps::Divide,Scanner::OperationPrecedence::Factor},
		//
		{BinaryExpression::bOps::FloorDivide,Scanner::OperationPrecedence::Factor},
		{BinaryExpression::bOps::Exponent,Scanner::OperationPrecedence::Power},
		{BinaryExpression::bOps::Modulo,Scanner::OperationPrecedence::Factor},
		//
		{BinaryExpression::bOps::BitwiseAnd,Scanner::OperationPrecedence::Bitwise},
		{BinaryExpression::bOps::BitwiseXor,Scanner::OperationPrecedence::Bitwise},
		{BinaryExpression::bOps::BitwiseOr,Scanner::OperationPrecedence::Bitwise},
		//
		{BinaryExpression::bOps::ShiftLeft,Scanner::OperationPrecedence::Bitwise},
		{BinaryExpression::bOps::ShiftRight,Scanner::OperationPrecedence::Bitwise},
		//
		{BinaryExpression::bOps::Concatenate,Scanner::OperationPrecedence::Concat},
		//
		{BinaryExpression::bOps::LessThan,Scanner::OperationPrecedence::Comparison},
		{BinaryExpression::bOps::LessEquals,Scanner::OperationPrecedence::Comparison},
		{BinaryExpression::bOps::Greater,Scanner::OperationPrecedence::Comparison},
		{BinaryExpression::bOps::GreaterEquals,Scanner::OperationPrecedence::Comparison},
		{BinaryExpression::bOps::Equals,Scanner::OperationPrecedence::Comparison},
		{BinaryExpression::bOps::NotEquals,Scanner::OperationPrecedence::Comparison},
		//
		{BinaryExpression::bOps::LogicalAnd,Scanner::OperationPrecedence::Logical},
		{BinaryExpression::bOps::LogicalOr,Scanner::OperationPrecedence::Logical},
		{BinaryExpression::bOps::LogicalXor,Scanner::OperationPrecedence::Logical}
	};

	enum class BlockType {
		Unknown, // ???
		Function,
		While,
		If,
		For
	};

	int tokenheader; // Which Token # are we on?
	//This whole thing is one big Turing machine, reading a roll of tokens in sequence, so having a master counter is important to the iteration.

	BinaryExpression::bOps readbOpOneChar (char* c, Token* t)
	{
		switch (c[0])
		{
			//This Switch tries to keep the binOps in the order in which they appear in JoaoGrammar.txt.
		case('+')://multichar needed
			return BinaryExpression::bOps::Add;
			break;
		case('-')://multichar needed
			return BinaryExpression::bOps::Subtract;
			break;
		case('*'):
			return BinaryExpression::bOps::Multiply;
			break;
		case('/'): //FIXME: The Scanner currently creates ambiguous Tokens, such that the Parser can't distinguish between "/apple/rotten/a / b" and "/apple/rotten/a/b".
			return BinaryExpression::bOps::Divide;
			break;
		case('^'):
			return BinaryExpression::bOps::Exponent;
			break;
		case('%'):
			return BinaryExpression::bOps::Modulo;
			break;
		case('&')://multichar needed
			return BinaryExpression::bOps::BitwiseAnd;
			break;
		case('~')://multichar needed
			return BinaryExpression::bOps::BitwiseXor;
			break;
		case('|')://multichar needed
			return BinaryExpression::bOps::BitwiseOr;
			break;
		case('>')://multichar needed
			return BinaryExpression::bOps::Greater;
			break;
		case('<')://multichar needed
			return BinaryExpression::bOps::LessThan;
			break;
		case('='):
			//We actually can deduce that this is == w/o looking at the second char in symbol. If this were assignment, we'd be reading a varstat, not an exp.
			return BinaryExpression::bOps::Equals;
			break;
		case('!'):
			//Related to the logic above, this has to be operator!=.
			//...that being said, we should genuinely check (FIXME) that these are accurate; they may be malformed symbols. Perhaps the check could even be in Scanner?
			return BinaryExpression::bOps::NotEquals;
			break;
		default:
			ParserError(t, "Unknown or unimplemented one-char operation!");
			break;
		}
		return BinaryExpression::bOps::NoOp;
	}
	BinaryExpression::bOps readbOpTwoChar(char* c, Token* t)
	{
		switch (c[1])
		{
			//This Switch tries to keep the binOps in the order in which they appear in JoaoGrammar.txt.
		case('+')://++
			ParserError(t, "Improper use of unary operator in BinaryExpression!");
			return BinaryExpression::bOps::Add;
			break;
		case('-')://--
			ParserError(t, "Improper use of unary operator in BinaryExpression!");
			return BinaryExpression::bOps::Subtract;
			break;
		case('/')://FIXME: The Scanner currently creates ambiguous Tokens, such that the Parser can't distinguish between "/apple/rotten/a / b" and "/apple/rotten/a/b".
			return BinaryExpression::bOps::FloorDivide;
			break;
		case('&')://&&
			return BinaryExpression::bOps::LogicalAnd;
			break;
		case('~')://~~
			return BinaryExpression::bOps::LogicalXor;
			break;
		case('|')://||
			return BinaryExpression::bOps::LogicalOr;
			break;
		case('>')://>>
			return BinaryExpression::bOps::ShiftRight;
			break;
		case('<')://<<
			return BinaryExpression::bOps::ShiftLeft;
			break;
		case('='):
			switch (c[0])
			{
			case('>'):// >=
				return BinaryExpression::bOps::GreaterEquals;
			case('<'):// <=
				return BinaryExpression::bOps::LessEquals;
			case('!'): // !=
				return BinaryExpression::bOps::NotEquals;
			case('='): // == 
				return BinaryExpression::bOps::Equals;
			default:
				ParserError(t, "Unknown or unimplemented equivalence operation!");
				break;
			}
			break;
		default:
			ParserError(t, "Unknown or unimplemented two-char operation!" + t->dump());
			break;
		}
		return BinaryExpression::bOps::NoOp;
	}

	ASTNode* readlvalue(int,int);
	ASTNode* readBinExp(Scanner::OperationPrecedence,int,int,bool);
	ASTNode* readExp(int,int,bool);
	Construction* readConstruction(int, int);

	//Here-there-update; does not update if no last bracket found
	std::vector<Expression*> readBlock(BlockType, int, int); 

	//Here-there-update; does not update if no last bracket found
	std::vector<LocalAssignmentStatement*> readClassDef(std::string, int, int);

	//Reads a strongly-expected LocalAssignment (of form, "Value x = 3" or whatever). Does not consume a semicolon.
	LocalAssignmentStatement* readLocalAssignment(LocalType, int, int);

	AssignmentStatement::aOps readaOp(int here = 0)
	{
#ifdef LOUD_TOKENHEADER
		std::cout << "readaOp starting at " << std::to_string(tokenheader) << std::endl;
#endif
		Token* t;
		if (here)
			t = tokens[here];
		else
			t = tokens[tokenheader];

		if (t->class_enum() != Token::cEnum::SymbolToken)
		{
			ParserError(t, "Unexpected Token when aOp was expected!");
			if(!here)
				++tokenheader;
			return AssignmentStatement::aOps::NoOp;
		}
		SymbolToken st = *static_cast<SymbolToken*>(t);

		char* c = st.get_symbol();

		if (st.len == 1)
		{
			if (c[0] == '=')
			{
				if(!here)
					++tokenheader;
				return AssignmentStatement::aOps::Assign;
			}
			else
			{
				ParserError(t, "Unexpected single-char SymbolToken; aOp was expected!");
			}
		}

		ParserError(t, "Two-char assignment operations are not implemented!");

		if(!here)
			++tokenheader;
#ifdef LOUD_TOKENHEADER
		std::cout << "readaOp setting tokenheader to " << std::to_string(tokenheader) << std::endl;
#endif
		return AssignmentStatement::aOps::NoOp;
	}

	std::string readIdentifierStr(bool is_word, int here, int there) // false if token is pointing at symbol
	{
#ifdef LOUD_TOKENHEADER
		std::cout << "readIdentifierStr starting at " << std::to_string(here) << std::endl;
#endif
		Token* t = tokens[here];

		if (!is_word)
		{
			ParserError(t, "Non-local identifiers are not implemented!");
			return "";
		}
		assert(t->class_enum() == Token::cEnum::WordToken);

		WordToken wt = *static_cast<WordToken*>(t);
		tokenheader = here + 1;
#ifdef LOUD_TOKENHEADER
		std::cout << "readIdentifierStr setting tokenheader to " << std::to_string(tokenheader) << std::endl;
#endif
		return wt.word;
	}

	// If no args, assumes tokenheader is pointing where it should. Increments tokenheader when given no args.
	void consume_semicolon(Token* t = nullptr) 
	{
		if (!t)
		{
			t = tokens[tokenheader];
			++tokenheader;
		}

		switch (t->class_enum()) // This could be an if-statement but I've been switching like this everywhere to the point it feels like poor style to not do it here
		{
		case(Token::cEnum::EndLineToken):
			return;
		default:
			//std::cout << "Stalling...le attempting to consumle attempting to consumle attempting to consumle attempting to consumle attempting to consumle attempting to consumle attempting to consum";
			ParserError(t, "Unexpected Token while attempting to consume EndLineToken!");
		}
	}

	void consume_paren(bool open, Token* t = nullptr)
	{
		if (!t)
		{
			t = tokens[tokenheader];
			++tokenheader;
		}

		switch (t->class_enum()) // This could be an if-statement but I've been switching like this everywhere to the point it feels like poor style to not do it here
		{
		case(Token::cEnum::PairSymbolToken):
		{
			PairSymbolToken pst = *static_cast<PairSymbolToken*>(t);
			if (pst.is_start != open || pst.t_pOp != PairSymbolToken::pairOp::Paren)
			{
				ParserError(t, "Unexpected PairSymbolToken while attempting to consume open Paren!");
			}
			return;
		}
		default:
			ParserError(t, "Unexpected Token while attempting to consume open Paren!");
		}
	}

	void consume_open_brace(int here)
	{
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
	}

	size_t find_first_semicolon(int here, int there)
	{
		for (int where = here; where <= there; ++where)
		{
			Token* t = tokens[where];

			if (t->class_enum() == Token::cEnum::EndLineToken)
				return where;
		}
		ParserError(tokens[here], "Failed to find expected semicolon!");
		return tokens.size() - 1;
	}
	
protected:
	//Does the nitty-gritty of filling out the function and objecttype tables.
	void generate_object_tree(std::vector<ClassDefinition*>&);
public:
	Parser(Scanner&t)
		:tokens(t.tokens)
		,lowest_ops(t.lowest_ops)
	{
		grammarstack.push_front(GrammarState::program);
	}
	void ParserError()
	{
		std::cout << "PARSER_ERROR: UNKNOWN!";
		exit(1);
	}
	void ParserError(Token* t, std::string what)
	{
		//This is just a basic setup while everything else is fleshed out.
		std::cout << "PARSER_ERROR: " << what << "\n";
		if (t)
			std::cout << t->dump();
		else
			std::cout << "ERROR_ERROR: No Token pointer provided to ParserError()!\n";
		exit(1);
	}
	void ParserError(Token& t, std::string what)
	{
		//This is just a basic setup while everything else is fleshed out.
		std::cout << "PARSER_ERROR: " << what << "\n";
		exit(1);
	}
	Program parse();
};