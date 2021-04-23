#pragma once

#include "AST.h"
#include "Object.h"
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
		{BinaryExpression::bOps::Exponent,Scanner::OperationPrecedence::Factor},
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

	std::string read_dir_name()
	{
		std::string str = "";

		bool looking_for_slash = true; // Part of the alternating flow control of, looking for a slash then a word then a slash then a word.
		Token* t = &Token();
		for (; tokenheader < tokens.size(); ++tokenheader)
		{
			t = tokens[tokenheader];
			switch (t->class_enum())
			{
			case(Token::cEnum::SymbolToken):
			{
				if (!looking_for_slash)
				{
					ParserError(t, "Unexpected symbol found in directory path!");
					return str;
				}
				SymbolToken st = *static_cast<SymbolToken*>(t); // Allah
				char* symb = st.get_symbol();
				if (symb[0] != '/' || symb[1] != '\0')
				{
					ParserError(t, "Unexpected symbol found in directory path! (Expecting '/', got something else)");
				}
				//So, slash found, I guess.
				str.push_back('/');
				//Consume it by flipping the "looking_for_slash bit" and continuing.
				looking_for_slash = false;
				continue;
			}
			case(Token::cEnum::WordToken):
			{
				if (looking_for_slash)
				{
					ParserError(t, "Unexpected word found in directory path!");
					return str;
				}

				// I can't think of any other condition that'd make this bad input, so...
				WordToken wt = *static_cast<WordToken*>(t);
				str.append(wt.word);

				looking_for_slash = true;
				continue;
			}
			default: // Found something weird!
				//We'll optimistically assume the weird new thing is supposed to be there and just return the directory string as we have it.
				return str;
			}
		}
		//... you can't end a program with, fucking, a directory-name thing.
		ParserError(t, "Unfinished block near directory path!");

		return str;
	}

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

	ASTNode* readlvalue(Token*);
	ASTNode* readBinExp(Scanner::OperationPrecedence,int,int);
	ASTNode* readExp(Token*, bool);

	AssignmentStatement::aOps readaOp()
	{
		Token* t = tokens[tokenheader];

		if (t->class_enum() != Token::cEnum::SymbolToken)
		{
			ParserError(t, "Unexpected Token when aOp was expected!");
			++tokenheader;
			return AssignmentStatement::aOps::NoOp;
		}
		SymbolToken st = *static_cast<SymbolToken*>(t);

		char* c = st.get_symbol();

		if (st.len == 1)
		{
			if (c[0] == '=')
			{
				++tokenheader;
				return AssignmentStatement::aOps::Assign;
			}
			else
			{
				ParserError(t, "Unexpected single-char SymbolToken; aOp was expected!");
			}
		}

		ParserError(t, "Two-char assignment operations are not implemented!");

		++tokenheader;
		return AssignmentStatement::aOps::NoOp;
	}

	std::string readIdentifierStr(bool is_word, Token* t = nullptr) // false if token is pointing at symbol
	{
		t = (t ? t : tokens[tokenheader]);

		if (!is_word)
		{
			ParserError(tokens[tokenheader], "Non-local identifiers are not implemented!");
			return "";
		}
		assert(t->class_enum() == Token::cEnum::WordToken);

		WordToken wt = *static_cast<WordToken*>(t);

		return wt.word;
	}

	std::vector<Expression*> readBlock(BlockType);
	
protected:
	void ParserError()
	{
		std::cout << "PARSER_ERROR: UNKNOWN!";
		exit(1);
	}
	void ParserError(Token* t, std::string what)
	{
		//This is just a basic setup while everything else is fleshed out.
		std::cout << "PARSER_ERROR: " << what << "\n";
		std::cout << t->dump();
		exit(1);
	}
	void ParserError(Token& t, std::string what)
	{
		//This is just a basic setup while everything else is fleshed out.
		std::cout << "PARSER_ERROR: " << what << "\n";
		exit(1);
	}
public:
	Parser(Scanner&t)
		:tokens(t.tokens)
		,lowest_ops(t.lowest_ops)
	{
		grammarstack.push_front(GrammarState::program);
	}
	Program parse();
};