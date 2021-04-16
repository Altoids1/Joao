#pragma once

#include "Forward.h"

#define NAME_CONST_METHODS(the_thing) virtual cEnum class_enum() const override { return cEnum::##the_thing; } \
						  virtual std::string class_name() const override { return #the_thing; }

/*
The SCANNER is a device that takes in the raw text file and outputs a system of tokens which can be parsed by the PARSER.
*/
class Token
{

public:
	enum class cEnum {
		Token,
		EndLineToken,
		NumberToken,
		SymbolToken,
		WordToken,
		StringToken,
		PairSymbolToken,
		KeywordToken,
		LiteralToken
	};
	uint32_t line;
	Token()
	{
		std::cout << "Why am I default constructed???";
		exit(1);
	}
	Token(uint32_t l)
		:line(l)
	{

	}

	virtual std::string dump() {
		return "LINE: " + std::to_string(line) + "NULL_TOKEN";
	}

	virtual cEnum class_enum() const { return cEnum::Token; }
	virtual std::string class_name() const { return "Token"; }
};

class EndLineToken final : public Token
{
public:
	EndLineToken(uint32_t l)
	{
		line = l;
	}
	NAME_CONST_METHODS(EndLineToken);
};

class NumberToken final : public Token
{
	union {
		double as_double;
		int as_int;
	}num;
	bool is_double;
public:
	NumberToken(uint32_t& l, double d)
	{
		line = l;
		num.as_double = d;
		is_double = true;
	}
	NumberToken(uint32_t& l, int i)
	{
		line = l;
		num.as_int = i;
		is_double = false;
	}

	virtual std::string dump() override
	{
		return "LINE: " + std::to_string(line) + std::string("NUMBER: ") + std::to_string(is_double ? num.as_double : num.as_int); // god.
	}
	NAME_CONST_METHODS(NumberToken);
};

class SymbolToken final : public Token
{
	char symbol[2];
public:
	SymbolToken(uint32_t& l, char symb, char symb2 = '\0')
	{
		line = l;
		symbol[0] = symb;
		symbol[1] = symb2;
	}

	char* get_symbol()
	{
		return symbol;
	}

	virtual std::string dump() override {
		std::string str = "";
		str.push_back(symbol[0]);
		if (symbol[1] != '\0')
		{
			str.push_back(symbol[1]);
		}
		return "LINE: " + std::to_string(line) + std::string("SYMBOL: ") + str;
	}
	NAME_CONST_METHODS(SymbolToken);
};

class WordToken final : public Token
{
public:
	std::string word;
	WordToken(uint32_t& l, std::string w)
	{
		line = l;
		word = w;
	}

	virtual std::string dump() override
	{
		return "LINE: " + std::to_string(line) + std::string("WORD: ") + word;
	}
	NAME_CONST_METHODS(WordToken);
};

class StringToken final : public Token
{
	std::string word;
public:
	StringToken(uint32_t& l, std::string w)
	{
		line = l;
		word = w;
	}
	virtual std::string dump() override
	{
		return "LINE: " + std::to_string(line) + std::string("STRING: ") + word;
	}
	NAME_CONST_METHODS(StringToken);
};

class PairSymbolToken final : public Token
{
public:
	enum class pairOp : uint8_t
	{
		Brace,
		Bracket,
		Paren
	}t_pOp;
	bool is_start = false; // FALSE if it's an end pairlet, TRUE if it's the start of a pair
	PairSymbolToken(uint32_t& l, char c)
	{
		line = l;
		switch (c)
		{
		case('{'):
			is_start = true;
		case('}'):
			t_pOp = pairOp::Brace;
			break;

		case('['):
			is_start = true;
		case(']'):
			t_pOp = pairOp::Bracket;
			break;

		case('('):
			is_start = true;
		case(')'):
			t_pOp = pairOp::Paren;
			break;

		default:
			std::cout << "Unknown character given to PairSymbolToken!";
			exit(1);
		}
	}
	virtual std::string dump() override
	{
		std::string str;
		switch (t_pOp)
		{
		case(pairOp::Brace):
			is_start ? str = "{" : str = "}";
		case(pairOp::Bracket):
			is_start ? str = "[" : str = "]";
		case(pairOp::Paren):
			is_start ? str = "(" : str = ")";
		}
		return "LINE: " + std::to_string(line) + std::string("PAIRSYMBOL: ") + str;
	}
	NAME_CONST_METHODS(PairSymbolToken);
};

class KeywordToken : public Token
{
public:
	enum class Key {
		If,
		Elseif,
		Else,
		While,
		For,
		Return,
		Break
	}t_key;

	KeywordToken(uint32_t linenum, Key k)
	{
		line = linenum;
		t_key = k;
	}

	NAME_CONST_METHODS(KeywordToken);
};

class LiteralToken : public Token
{
public:
	enum class Literal {
		Null,
		False,
		True,
	}t_literal;
	LiteralToken(uint32_t linenum, Literal k)
	{
		line = linenum;
		t_literal = k;
	}

	NAME_CONST_METHODS(LiteralToken);
};

class Scanner
{
	uint32_t linenum = 0;
	std::string line;
	std::vector<Token*> tokens;

	const std::unordered_map<std::string, KeywordToken::Key> keywordhash = {
		{"if",KeywordToken::Key::If},
		{"while",KeywordToken::Key::While},
		{"for",KeywordToken::Key::For},
		{"return",KeywordToken::Key::Return}
	};
	const std::unordered_map<std::string, LiteralToken::Literal> literalhash = {
		{"null",LiteralToken::Literal::Null},
		{"false",LiteralToken::Literal::False},
		{"true",LiteralToken::Literal::True}
	};

	void ScannerError()
	{
		std::cout << "SCANNER_ERROR: UNKNOWN!";
		exit(1);
	}
	void ScannerError(int column_num, std::string what)
	{
		//This is just a basic setup while everything else is fleshed out.
		std::cout << "SCANNER_ERROR: " << what << "\n";
		exit(1);
	}
	void append(Token* t)
	{
		Token* T = new Token(*t);
		tokens.push_back(T);
	}
	void append(Token& t)
	{
		Token* T = new Token(t);
		tokens.push_back(T);
	}
	void makeEndline() // This is its own function to allow for the read___() functions to quickly call it when they accidentally tread onto a semicolon while deciphering a token.
	{
		Token* t = &EndLineToken(linenum);
		linenum++;
		append(t);
	}
	void makeNumber(bool is_double, std::string& str, int base = 10)
	{
		if (is_double)
		{
			double d = std::stod(str); // Because of how stringent we were with assembling this string, we can pretty confidently do this w/o sanity-checking;
			//stod() will pretty much for-sure give us something sensible.
			NumberToken nt = NumberToken(linenum, d);
			append(&nt);
		}
		else
		{
			int i = std::stoi(str, nullptr, base); //^^^ Ditto for stoi().
			NumberToken nt = NumberToken(linenum, i);
			append(&nt);
		}
	}
	void makeWord(std::string str)
	{
		//first check if this is a keyword
		if (keywordhash.count(str))
		{
			KeywordToken kt = KeywordToken(linenum, keywordhash.at(str));
			append(&kt);
			return;
		}
		//then check if this is a literal (like 'true')

		if (literalhash.count(str))
		{
			LiteralToken lt = LiteralToken(linenum, literalhash.at(str));
			append(&lt);
			return;
		}


		//otherwise, do the normal business
		WordToken wt = WordToken(linenum, str);
		append(&wt);
	}
	int readString(int);
	int readNumber(int);
	int readPairSymbol(int);
	int readSymbol(int);
	int readWord(int);
public:
	void scan(std::ifstream&);

	friend class Parser;
};