#pragma once

#include "Forward.h"

#include "SharedEnums.h"
#include "Directory.h"

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
		LiteralToken,
		LocalTypeToken,
		DirectoryToken,
		ConstructionToken,
		ParentToken,
		GrandparentToken,
		MemberToken,
		CommaToken
	};
	uint32_t line;
	uint32_t syntactic_line;
	Token()
	{
#ifdef LOUD_DEFAULT_CONSTRUCT
		std::cout << class_name() << " was default-constructed!" << std::endl;
#endif
		//Weird.
	}
	Token(uint32_t l, uint32_t sl)
		:line(l)
		,syntactic_line(sl)
	{

	}

	virtual std::string dump() {
		return "LINE: " + std::to_string(line) + "," + std::to_string(syntactic_line) + " " +  class_name();
	}

	virtual cEnum class_enum() const { return cEnum::Token; }
	virtual std::string class_name() const { return "Token"; }
};

class EndLineToken final : public Token
{
public:
	EndLineToken(uint32_t l, uint32_t sl)
	{
		line = l;
		syntactic_line = sl;
	}
	NAME_CONST_METHODS(EndLineToken);
};

class NumberToken final : public Token
{
public:
	union {
		double as_double;
		int as_int;
	}num;
	bool is_double;
	NumberToken(uint32_t& l, uint32_t sl, double d)
	{
		line = l;
		syntactic_line = sl;
		num.as_double = d;
		is_double = true;
	}
	NumberToken(uint32_t& l, uint32_t sl,  int i)
	{
		line = l;
		syntactic_line = sl;
		num.as_int = i;
		is_double = false;
	}

	virtual std::string dump() override
	{
		return "LINE: " + std::to_string(line) + "," + std::to_string(syntactic_line) + std::string(" NUMBER: ") + std::to_string(is_double ? num.as_double : num.as_int); // god.
	}
	NAME_CONST_METHODS(NumberToken);
};

class SymbolToken final : public Token
{
	char symbol[2];
public:
	char len;
	SymbolToken(uint32_t& l, uint32_t sl, char symb)
	{
		line = l;
		syntactic_line = sl;
		symbol[0] = symb;
		symbol[1] = '\0';
		len = 1;
	}
	SymbolToken(uint32_t& l, uint32_t sl, char symb, char symb2)
	{
		line = l;
		syntactic_line = sl;
		symbol[0] = symb;
		symbol[1] = symb2;
		if (symb2 == '\0')
		{
			len = 1;
		}
		else
		{
			len = 2;
		}
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
		return "LINE: " + std::to_string(line) + "," + std::to_string(syntactic_line) + std::string(" SYMBOL: (") + str + std::string(") LEN: ") + std::to_string(int(len));
	}
	NAME_CONST_METHODS(SymbolToken);
};

class WordToken final : public Token
{
public:
	std::string word;
	WordToken(uint32_t& l, uint32_t sl, std::string w)
	{
		line = l;
		syntactic_line = sl;
		word = w;
	}

	virtual std::string dump() override
	{
		return "LINE: " + std::to_string(line) + "," + std::to_string(syntactic_line) + std::string(" WORD: ") + word;
	}
	NAME_CONST_METHODS(WordToken);
};

class StringToken final : public Token
{
public:
	std::string word;
	StringToken(uint32_t& l, uint32_t sl, std::string w)
	{
		line = l;
		syntactic_line = sl;
		word = w;
	}
	virtual std::string dump() override
	{
		return "LINE: " + std::to_string(line) + "," + std::to_string(syntactic_line) + std::string(" STRING: ") + word;
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
	PairSymbolToken(uint32_t l, uint32_t sl, char c)
	{
		line = l;
		syntactic_line = sl;

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
			break;
		case(pairOp::Bracket):
			is_start ? str = "[" : str = "]";
			break;
		case(pairOp::Paren):
			is_start ? str = "(" : str = ")";
			break;
		default:
			str = "UNKNOWN???";
		}

		return "LINE: " + std::to_string(line) + "," + std::to_string(syntactic_line) + std::string(" PAIRSYMBOL: ") + str;
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

	KeywordToken(uint32_t linenum, uint32_t sl, Key k)
	{
		line = linenum;
		syntactic_line = sl;
		t_key = k;
	}

	NAME_CONST_METHODS(KeywordToken);
};

class LocalTypeToken : public Token 
{
public:
	LocalType t_type;

	LocalTypeToken(uint32_t l, uint32_t sl, LocalType ty)
		:t_type(ty)
	{
		line = l;
		syntactic_line = sl;
	}


	NAME_CONST_METHODS(LocalTypeToken);
};

class LiteralToken : public Token
{
public:
	enum class Literal {
		Null,
		False,
		True,
	}t_literal;
	LiteralToken(uint32_t linenum, uint32_t sl, Literal k)
	{
		line = linenum;
		syntactic_line = sl;
		t_literal = k;
	}

	NAME_CONST_METHODS(LiteralToken);
};

//The Scanner actually vaguely knowing the language it scans for! Helps interpreter "/apple/green/rotten" as one big token, and disambiguates it in tokenspace from "/ apple / green / rotten"
class DirectoryToken : public Token
{
public:
	std::string dir;
	DirectoryToken(uint32_t linenum, uint32_t sl, std::string k)
	{
		line = linenum;
		syntactic_line = sl;
		dir = k;
	}
	virtual std::string dump() override
	{
		return "LINE: " + std::to_string(line) + "," + std::to_string(syntactic_line) + std::string(" DIRECTORY: ") + dir;
	}
	NAME_CONST_METHODS(DirectoryToken);
};

//Practically identical to a DirectoryToken in structure, just marks that it was /apple/rotten/New instead of /apple/rotten
class ConstructionToken : public Token
{
public:
	std::string dir;
	ConstructionToken(uint32_t linenum, uint32_t sl, std::string k)
	{
		line = linenum;
		syntactic_line = sl;
		dir = k;
	}
	virtual std::string dump() override
	{
		return "LINE: " + std::to_string(line) + "," + std::to_string(syntactic_line) + std::string(" CONSTRUCTION: ") + dir;
	}
	NAME_CONST_METHODS(ConstructionToken);
};

class ParentToken : public Token
{

public:
	ParentToken(uint32_t l, uint32_t sl)
		:Token(l,sl)
	{

	}
	NAME_CONST_METHODS(ParentToken);
};

class GrandparentToken : public Token
{
public:
	GrandparentToken(uint32_t l, uint32_t sl)
		:Token(l, sl)
	{

	}
	NAME_CONST_METHODS(GrandparentToken);
};

class MemberToken : public Token
{
public:
	MemberToken(uint32_t l, uint32_t sl)
		:Token(l, sl)
	{

	}
	NAME_CONST_METHODS(MemberToken);
};

class CommaToken : public Token
{
public:
	CommaToken(uint32_t l, uint32_t sl)
		:Token(l, sl)
	{

	}
	NAME_CONST_METHODS(CommaToken);
};

class Scanner
{
	enum class OperationPrecedence : uint8_t { // First is done first, last is done last
		NONE, // Used by the Scanner to report that no operation takes place within an expression.
		Power, // ^
		Unary, // - ! #
		Factor, // / *
		Term, // + - 
		Concat, // .. (Lua does this so we're doing this)
		Bitwise, // & ~ |
		Comparison, // == != >= =< > <
		Logical //  && || ~~
	};

	const std::unordered_map<std::string, OperationPrecedence> str_to_precedence = {
		{"!",OperationPrecedence::Unary},
		{"~",OperationPrecedence::Unary},
		{"#",OperationPrecedence::Unary},
		//
		{"+",OperationPrecedence::Term},
		{"-",OperationPrecedence::Term},
		{"*",OperationPrecedence::Factor},
		{"/",OperationPrecedence::Factor},
		//
		{"//",OperationPrecedence::Factor},
		{"^",OperationPrecedence::Power},
		{"%",OperationPrecedence::Factor},
		//
		{"&",OperationPrecedence::Bitwise},
		{"~",OperationPrecedence::Bitwise},
		{"|",OperationPrecedence::Bitwise},
		//
		{">>",OperationPrecedence::Bitwise},
		{"<<",OperationPrecedence::Bitwise},
		//
		{"..",OperationPrecedence::Concat},
		//
		{"<",OperationPrecedence::Comparison},
		{"<=",OperationPrecedence::Comparison},
		{">",OperationPrecedence::Comparison},
		{">=",OperationPrecedence::Comparison},
		{"==",OperationPrecedence::Comparison},
		{"!=",OperationPrecedence::Comparison},
		//
		{"&&",OperationPrecedence::Logical},
		{"||",OperationPrecedence::Logical},
		{"~~",OperationPrecedence::Logical},
	};

	enum class ScanError {
		Unknown,
		UnterminatedString,
		UnknownCharacter,
		MalformedNumber,
		MalformedString,
		MalformedLongComment,
		MalformedDirectory,
		BadDaddy,
		BadGrandpa
	};

	uint32_t linenum = 0;
	uint32_t syntactic_linenum = 0;

	std::string line;
	std::vector<Token*> tokens;
	std::vector<OperationPrecedence> lowest_ops; // A vector which stores the lowest-priority operation used in the syntactic line its index points to.

	OperationPrecedence lowop = OperationPrecedence::NONE;

	const std::unordered_map<std::string, KeywordToken::Key> keywordhash = {
		{"if",KeywordToken::Key::If},
		{"while",KeywordToken::Key::While},
		{"for",KeywordToken::Key::For},
		{"return",KeywordToken::Key::Return},
		{"break",KeywordToken::Key::Break}
	};
	const std::unordered_map<std::string, LiteralToken::Literal> literalhash = {
		{"null",LiteralToken::Literal::Null},
		{"false",LiteralToken::Literal::False},
		{"true",LiteralToken::Literal::True}
	};

	const std::unordered_map<std::string, LocalType> typehash = {
		{"Value",LocalType::Value},
		{"Number",LocalType::Number},
		{"Object",LocalType::Object},
		{"String",LocalType::String},
		{"Boolean",LocalType::Boolean},
		{"local",LocalType::Local}
	};

	void ScannerError(unsigned int column_num = 0, ScanError what = ScanError::Unknown)
	{
		std::string msg = "";

		unsigned int spaces = (column_num) ? (column_num - 1) : 0;
		std::string squiggly = std::string(spaces, ' ') + std::string("^");;  // A string to print under a print-out of the line we're looking at,
		//which points to the offending character.

		switch (what)
		{
		case(ScanError::UnterminatedString):
			msg =  "SCANNER_ERROR: Unterminated String!";
			break;
		case(ScanError::UnknownCharacter):
			msg = "SCANNER_ERROR: Unknown character!\nCharacter is: " + std::to_string(int(line[column_num]));
			break;
		case(ScanError::MalformedNumber):
			msg = "SCANNER_ERROR: Malformed Number!";
			break;
		case(ScanError::MalformedString):
			msg = "SCANNER_ERROR: Malformed String!";
			break;
		case(ScanError::MalformedLongComment):
			msg = "SCANNER_ERROR: Long comments cannot begin on lines containing functional code!";
			/*
			Picky? Yes.

			Good for code structure? Definitely; I don't like the idea of having to read random fucking longcomments in the middle of code,
			just makes the Parser slower to accomodate horrendous style.
			*/
			break;
		case(ScanError::MalformedDirectory):
			msg = "SCANNER_ERROR: Malformed Directory!";
			break;
		case(ScanError::BadDaddy):
			msg = "SCANNER_ERROR: Improper Parent Access!"; // don't you access your daddy like that
			break;
		case(ScanError::BadGrandpa):
			msg = "SCANNER_ERROR: Improper Grandparent Access!";
			break;
		default:
		case(ScanError::Unknown):
			std::cout << "SCANNER_ERROR: UNKNOWN!"; // This is an error at printing an error. So meta!
			exit(1);
		}

		std::cout << msg << std::endl << string::replace_all(line,'\t',' ') << std::endl << squiggly << std::endl;
		exit(1);
	}
	void append(Token* t)
	{
		tokens.push_back(t);
	}
	void makeEndline() // This is its own function to allow for the read___() functions to quickly call it when they accidentally tread onto a semicolon while deciphering a token.
	{
		Token* t = new EndLineToken(linenum,syntactic_linenum);
		append(t);
		
		//Order-of-operation optimization stuffs
		lowest_ops.push_back(lowop);
		lowop = OperationPrecedence::NONE;
		++syntactic_linenum;
	}
	void makeNumber(bool is_double, std::string& str, int base = 10)
	{
		if (is_double)
		{
			double d = std::stod(str); // Because of how stringent we were with assembling this string, we can pretty confidently do this w/o sanity-checking;
			//stod() will pretty much for-sure give us something sensible.
			NumberToken* nt = new NumberToken(linenum, syntactic_linenum, d);
			append(nt);
		}
		else
		{
			int i = std::stoi(str, nullptr, base); //^^^ Ditto for stoi().
			NumberToken* nt = new NumberToken(linenum, syntactic_linenum, i);
			append(nt);
		}
	}
	void makeWord(std::string str)
	{
		if (str.empty())
			ScannerError();

		//first check if this is a keyword
		if (keywordhash.count(str))
		{
			KeywordToken* kt = new KeywordToken(linenum, syntactic_linenum, keywordhash.at(str));
			append(kt);
			return;
		}
		//then check if this is a literal (like 'true' or 'null')

		if (literalhash.count(str))
		{
			LiteralToken* lt = new LiteralToken(linenum, syntactic_linenum, literalhash.at(str));
			append(lt);
			return;
		}

		//then check if this is a Type keyword (like 'Value' or 'Number')

		if (typehash.count(str))
		{
			LocalTypeToken* ltt = new LocalTypeToken(linenum, syntactic_linenum, typehash.at(str));
			append(ltt);
			return;
		}


		//otherwise, do the normal business
		WordToken* wt = new WordToken(linenum, syntactic_linenum, str);
		append(wt);
	}
	std::string getWord(int&);

	int readString(int);
	int readNumber(int);
	int readPairSymbol(int);
	int readSymbol(int,std::ifstream&);
	int readWord(int);
	int readComment(int,std::ifstream&);
	int readSlash(int,std::ifstream&);

	static std::string precedence_tostring(OperationPrecedence op)
	{
		switch (op)
		{
		case(OperationPrecedence::NONE):
			return "NONE";
		case(OperationPrecedence::Power):
			return "Power";
		case(OperationPrecedence::Unary):
			return "Unary";
		case(OperationPrecedence::Factor):
			return "Factor";
		case(OperationPrecedence::Term):
			return "Term";
		case(OperationPrecedence::Concat):
			return "Concat";
		case(OperationPrecedence::Bitwise):
			return "Bitwise";
		case(OperationPrecedence::Comparison):
			return "Comparison";
		case(OperationPrecedence::Logical):
			return "Logical";
		default:
			return "?????";
		}
	}
	
public:
	void scan(std::ifstream&);

	friend class Parser;
};