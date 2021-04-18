#pragma once

#include "AST.h"
#include "Object.h"
#include "Scanner.h"

template <class _Ty>
class Scope {
	/*
	This class can serve two functionalities, whose implementations are merged into one powerful class.
	1. This can be used by the Parser to keep track of the Object or Function tree system, to be later collapsed into Program::definedFunctions and Program::definedObjTypes
	2. This can be used by the Interpreter to keep track of scoped variables (so, Values), allowing for their access and dismissal as it enters and exits various Scopes.
	*/
	std::unordered_map < std::string, _Ty*> ScopeTable;
	Scope<_Ty>* parent_scope = nullptr; // A higher-level scope to query against if we can't find our own stuff.
public:
	std::string scopename = "/";

	Scope(std::string sc, Scope* s = nullptr)
		:scopename(sc),
		parent_scope(s)
	{

	}
	std::string get_name()
	{
		return scopename;
	}

	_Ty* get(std::string name)
	{
		if (ScopeTable.count(name)) // If we have it
		{
			return ScopeTable[name]; // Give it
		}
		//else...
		if (parent_scope != nullptr) // If we have a parent scope 
		{
			return parent_scope->get(name); // ask them
		}
		//else...
		return nullptr; // Give up.
	}
	void set(std::string name, _Ty &t)
	{
		_Ty* T = new _Ty(t); // Make a dynamically-allocated copy
		ScopeTable[name] = T; // and store its pointery-doodle.
	}


	/*
	So this function is kinda interesting.
	The default structure of this thing is to be used as a sort of "linked list" that does these (expensive!) recursive calls to find things.
	This function allows the conversion of the linked list into one big, fat hashtable that stores everything in strings that have a directory structure,
	with the names of each directory being the Scope::scopename things of each Scope.
	*/
	/*
	std::unordered_map<std::string, _Ty*>* rdump()
	{
		std::unordered_map<std::string, _Ty*> master = ScopeTable;
		if (parent_scope)
		{
			std::unordered_map<std::string, _Ty*>* parent_ptr = parent_scope->rdump();
			master.insert()
		}
		return &master;
	}
	*/
};

class Program // this is pretty much what Parser is supposed to output, and what Interpreter is supposed to take as input.
{
	std::unordered_map<std::string, Function*> definedFunctions;
	std::unordered_map<std::string, ObjectType*> definedObjTypes;
	Interpreter* myinterp{ nullptr };
public:
	enum class ErrorCode : int {
		BadArgType,
		NotEnoughArgs
	};
	Program()
	{
		//Construct all the native functions
		construct_natives();
	}
	Program(Function *f)
	{
		definedFunctions["main"] = f;
		construct_natives();
	}
	void set_interp(Interpreter& interp)
	{
		myinterp = &interp;
	}
	void construct_natives();

	// I wanna point out that this is distinct from Interpreter's version of this function; it's a raw call to a function's name, directory data and all, while Interpreter's resolves scope first.
	Function* get_func(std::string name)
	{
		if (!definedFunctions.count(name))
			return nullptr;
		return definedFunctions[name];
	}
	void set_func(std::string name, Function* f)
	{
		definedFunctions[name] = f;
	}

};

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

	ASTNode* readExp(Token* t, bool expecting_semicolon = true)
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
			ParserError(t,"Failed to comprehend lvalue of Expression!");
		}

		//Now lets see if there's a binary operator and, if so, construct a BinaryExpression() to return.

		++tokenheader;

		Token* t2 = tokens[tokenheader];

		

		BinaryExpression::bOps bippitybop = BinaryExpression::bOps::NoOp;
		if (t2->class_enum() == Token::cEnum::EndLineToken)
		{
			if (!expecting_semicolon)
			{
				ParserError(t2,"Unexpected semicolon in expression!");
			}
			return lvalue;
		}
		else if (t2->class_enum() == Token::cEnum::SymbolToken)
		{
			SymbolToken st = *static_cast<SymbolToken*>(t2);

			char* c = st.get_symbol();

			if (c[1] != '\0')
				ParserError(t2, "Binary Operations longer than one character are not implemented!");

			switch (c[0]) // FIXME: This fails to support multichar symbols.
			{
			//This Switch tries to keep the binOps in the order in which they appear in JoaoGrammar.txt.
			case('+')://multichar needed
				bippitybop = BinaryExpression::bOps::Add;
				break;
			case('-')://multichar needed
				bippitybop = BinaryExpression::bOps::Subtract;
				break;
			case('*'):
				bippitybop = BinaryExpression::bOps::Multiply;
				break;
			case('/'): //FIXME: The Scanner currently creates ambiguous Tokens, such that the Parser can't distinguish between "/apple/rotten/a / b" and "/apple/rotten/a/b".
				bippitybop = BinaryExpression::bOps::Divide;
				break;
			case('^'):
				bippitybop = BinaryExpression::bOps::Exponent;
				break;
			case('%'):
				bippitybop = BinaryExpression::bOps::Modulo;
				break;
			case('&')://multichar needed
				bippitybop = BinaryExpression::bOps::BitwiseAnd;
				break;
			case('~')://multichar needed
				bippitybop = BinaryExpression::bOps::BitwiseXor;
				break;
			case('|')://multichar needed
				bippitybop = BinaryExpression::bOps::BitwiseOr;
				break;
			case('>')://multichar needed
				bippitybop = BinaryExpression::bOps::Greater;
				break;
			case('<')://multichar needed
				bippitybop = BinaryExpression::bOps::LessThan;
				break;
			case('='):
				//We actually can deduce that this is == w/o looking at the second char in symbol. If this were assignment, we'd be reading a varstat, not an exp.
				bippitybop = BinaryExpression::bOps::Equals;
				break;
			case('!'):
				//Related to the logic above, this has to be operator!=.
				//...that being said, we should genuinely check (FIXME) that these are accurate; they may be malformed symbols. Perhaps the check could even be in Scanner?
				bippitybop = BinaryExpression::bOps::NotEquals;
				break;
			default:
			{
				ParserError(t2, "Unknown or unimplemented operation: " + t2->dump());
				break;
			}
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

	std::vector<Expression*> readBlock(BlockType bt) // Tokenheader state should point to the opening brace of this block.
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
					ReturnStatement* rs = new ReturnStatement(readExp(tokens[tokenheader],true));
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
			case(Token::cEnum::WordToken):
			case(Token::cEnum::StringToken):
			case(Token::cEnum::NumberToken):
			case(Token::cEnum::SymbolToken):
				//This is.. probably an stat.
				ParserError(t, "Statements that are not return statements are not implemented! " + t->class_name());
				continue;
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
	{
		grammarstack.push_front(GrammarState::program);
	}
	Program parse();
};