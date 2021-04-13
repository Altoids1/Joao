#pragma once

#include "Forward.h"

/*
The SCANNER is a device that takes in the raw text file and outputs a system of tokens which can be parsed by the PARSER.
*/
class Token
{
public:
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

	virtual std::string class_name() const { return "Token"; }
};

class EndLineToken final : public Token
{
public:
	EndLineToken(uint32_t l)
	{
		line = l;
	}
	virtual std::string class_name() const { return "EndLineToken"; }
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
	virtual std::string class_name() const override { return "NumberToken"; }
};

class SymbolToken final : Token
{
	char symbol[2];
public:
	SymbolToken(uint32_t& l, char symb, char symb2 = '\0')
	{
		line = l;
		symbol[0] = symb;
		symbol[1] = symb2;
	}

	virtual std::string dump() override;
	virtual std::string class_name() const override { return "SymbolToken"; }
};

class WordToken final : public Token
{
	std::string word;
public:
	WordToken(uint32_t& l, std::string w)
	{
		line = l;
		word = w;
	}

	virtual std::string dump() override;
	virtual std::string class_name() const override { return "WordToken"; }
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
	virtual std::string class_name() const override { return "StringToken"; }
};


class Scanner
{
	uint32_t linenum = 0;
	std::string line;
	std::vector<Token*> tokens;

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
	void makeNumber(bool is_double, std::string& str)
	{
		if (is_double)
		{
			double d = std::stod(str); // Because of how stringent we were with assembling this string, we can pretty confidently do this w/o sanity-checking;
			//stod() will pretty much for-sure give us something sensible.
			Token* t = &NumberToken(linenum, d);
			append(t);
		}
		else
		{
			int i = std::stoi(str); //^^^ Ditto for stoi().
			Token* t = &NumberToken(linenum, i);
			append(t);
		}
	}
	int readString(int);
	int readNumber(int);
public:
	void scan(std::ifstream&);
};