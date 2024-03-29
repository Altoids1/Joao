#pragma once

#include "Forward.h"

#include "SharedEnums.h"
#include "Directory.h"
#include "ImmutableString.h"
#include "Error.h"
#include "Terminal.h"

#define NAME_CONST_METHODS(the_thing) virtual cEnum class_enum() const override { return cEnum :: the_thing; } \
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
	virtual ~Token() = default;

	std::string dump() {
		return "Line " + std::to_string(line) + "," + std::to_string(syntactic_line) + ": " + dumpable_name();
	}
	virtual std::string dumpable_name() {
		return class_name();
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

	virtual std::string dumpable_name() override
	{
		return std::to_string(is_double ? num.as_double : num.as_int);
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

	virtual std::string dumpable_name() override {
		std::string str = "";
		str.push_back(symbol[0]);
		if (symbol[1] != '\0')
		{
			str.push_back(symbol[1]);
		}
		return std::string(" SYMBOL: (") + str + std::string(") LEN: ") + std::to_string(int(len));
	}
	NAME_CONST_METHODS(SymbolToken);
};

class WordToken final : public Token
{
public:
	ImmutableString word;
	WordToken(uint32_t l, uint32_t sl, const ImmutableString& w)
		:word(w)
	{
		line = l;
		syntactic_line = sl;
	}

	virtual std::string dumpable_name() override
	{
		return std::string(" WORD: ") + word.to_string();
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
	virtual std::string dumpable_name() override
	{
		return std::string(" STRING: ") + word;
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
			[[fallthrough]];
		case('}'):
			t_pOp = pairOp::Brace;
			break;

		case('['):
			is_start = true;
			[[fallthrough]];
		case(']'):
			t_pOp = pairOp::Bracket;
			break;

		case('('):
			is_start = true;
			[[fallthrough]];
		case(')'):
			t_pOp = pairOp::Paren;
			break;

		default:
			throw error::scanner("Unknown character given to PairSymbolToken!");
		}
	}
	virtual std::string dumpable_name() override
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

		return std::string(" PAIRSYMBOL: ") + str;
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
		In, // Used for foreaches; "for(key,val in arr)"
		Return,
		Break,
		Continue,
		Try,
		Catch,
		Throw,
		Const,
	}t_key;

	KeywordToken(uint32_t linenum, uint32_t sl, Key k)
	{
		line = linenum;
		syntactic_line = sl;
		t_key = k;
	}

	NAME_CONST_METHODS(KeywordToken);
	virtual std::string dumpable_name() override {
		std::string str = "KEYWORDTOKEN: ";
		switch (t_key) {
		case(Key::If):
			str += "'if'";
			break;
		case(Key::Elseif):
			str += "'elseif'";
			break;
		case(Key::Else):
			str += "'else'";
			break;
		case(Key::While):
			str += "'while'";
			break;
		case(Key::For):
			str += "'for'";
			break;
		case(Key::In):
			str += "'in'";
			break;
		case(Key::Return):
			str += "'return'";
			break;
		case(Key::Break):
			str += "'break'";
			break; // lol
		case(Key::Continue):
			str += "'continue'";
			break;
		case(Key::Try):
			str += "'try'";
			break;
		case(Key::Catch):
			str += "'Catch'";
			break;
		case(Key::Throw):
			str += "'throw'";
			break;
		case(Key::Const):
			str += "'const'";
			break;
		default:
			str += "???";
			break;
		}
		return str;
	}
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
	virtual std::string dumpable_name() override
	{
		return std::string(" DIRECTORY: ") + dir;
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
	virtual std::string dumpable_name() override
	{
		return std::string(" CONSTRUCTION: ") + dir;
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

	const Hashtable<std::string, OperationPrecedence> str_to_precedence = {
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
		UnterminatedLongComment,
		UnknownCharacter,
		MalformedNumber,
		MalformedString,
		MalformedLongComment,
		MalformedDirectory,
		BadDaddy,
		BadGrandpa,
		SwappedEqOp
	};

	const bool is_interactive;

	uint32_t linenum = 0;
	uint32_t syntactic_linenum = 0;

	std::string line;
	std::vector<Token*> tokens;
	std::vector<OperationPrecedence> lowest_ops; // A vector which stores the lowest-priority operation used in the syntactic line its index points to.

	OperationPrecedence lowop = OperationPrecedence::NONE;

	const Hashtable<std::string, KeywordToken::Key> keywordhash = {
		{"if",KeywordToken::Key::If},
		{"elseif",KeywordToken::Key::Elseif},
		{"else",KeywordToken::Key::Else},
		{"while",KeywordToken::Key::While},
		{"for",KeywordToken::Key::For},
		{"in",KeywordToken::Key::In},
		{"return",KeywordToken::Key::Return},
		{"break",KeywordToken::Key::Break},
		{"try",KeywordToken::Key::Try},
		{"catch",KeywordToken::Key::Catch},
		{"throw",KeywordToken::Key::Throw},
		{"continue",KeywordToken::Key::Continue},
		{"const",KeywordToken::Key::Const},
	};
	const Hashtable<std::string, LiteralToken::Literal> literalhash = {
		{"null",LiteralToken::Literal::Null},
		{"false",LiteralToken::Literal::False},
		{"true",LiteralToken::Literal::True}
	};

	const Hashtable<std::string, LocalType> typehash = {
		{"Value",LocalType::Value},
		{"Number",LocalType::Number},
		{"Object",LocalType::Object},
		{"String",LocalType::String},
		{"Boolean",LocalType::Boolean},
		{"local",LocalType::Local}
	};

	void ScannerError(unsigned int column_num = 0, ScanError what = ScanError::Unknown)
	{
		Terminal::SetColor(std::cerr, Terminal::Color::Red);
		Terminal::SetBold(std::cerr, true);
		std::cerr << "Scanner Error: ";
		Terminal::ClearFormatting(std::cerr);
		std::string msg;
		switch (what)
		{
		case(ScanError::UnterminatedString):
			msg = "Unterminated String!";
			break;
		case(ScanError::UnterminatedLongComment):
			msg = "Unterminated long comment!";
			break;
		case(ScanError::UnknownCharacter):
			msg = "Unknown, unexpected, or unsupported character! (Character code: " + std::to_string(int(line[column_num])) + ")";
			break;
		case(ScanError::MalformedNumber):
			msg = "Malformed Number!";
			break;
		case(ScanError::MalformedString):
			msg = "Malformed String!";
			break;
		case(ScanError::MalformedLongComment):
			msg = "Long comments cannot start nor end on lines containing functional code!";
			/*
			Picky? Yes.

			Good for code structure? Definitely; I don't like the idea of having to read random fucking longcomments in the middle of code.
			It makes the Scanner slower, just for the sake of accomodating horrendous style.
			*/
			break;
		case(ScanError::MalformedDirectory):
			msg = "Malformed Directory!";
			break;
		case(ScanError::BadDaddy):
			msg = "Improper Parent Access!"; // don't you access your daddy like that
			break;
		case(ScanError::BadGrandpa):
			msg = "Improper Grandparent Access!";
			break;
		case(ScanError::SwappedEqOp):
			msg = "Improper comparison operation! Did you mean " + std::string({line[column_num], line[column_num-1]}) + "?";
			break;
		default:
		case(ScanError::Unknown):
			std::cerr << "UNKNOWN!"; // This is an error at printing an error. So meta!
			exit(1);
		}
		unsigned int spaces = (column_num) ? (column_num - 1) : 0;
		std::string squiggly = std::string(spaces, ' ') + std::string("^");;
		//          ^ this is to help place a little caret to point at the offending token.
		// TODO: Add this sort of caret-ing to Parser errors, as well :)
		std::cout << msg << std::endl << string::replace_all(line,'\t',' ') << std::endl << squiggly << std::endl;
		if (!is_interactive)
#ifdef JOAO_SAFE
			throw error::scanner(msg);
#else
			exit(1);
#endif
		else
			is_malformed = true;
	}
	void append(Token* t)
	{
		tokens.push_back(t);
	}
	void makeEndline() // This is its own function to allow for the read___() functions to quickly call it when they accidentally tread onto a semicolon while deciphering a token.
	{
		append(new EndLineToken(linenum,syntactic_linenum));
		
		//Order-of-operation optimization stuffs
		lowest_ops.push_back(lowop);
		lowop = OperationPrecedence::NONE;
		++syntactic_linenum;
	}
	void makeNumber(int it, bool is_double, std::string& str, int base = 10)
	{
		try {
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
		catch (std::out_of_range except) {
			ScannerError(it, ScanError::MalformedNumber);
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
	std::string getString(int&);

	int readString(int);
	int readNumber(int);
	int readPairSymbol(int);
	int readEqSymbol(int, std::istream&);
	int readSymbol(int,std::istream&);
	int readWord(int);
	int readComment(int,std::istream&);
	int readSlash(int,std::istream&);
	
	//Checks if the portion of the string starting at $start and ending at $end is all whitespace.
	//DOES NOT CHECK IF the index args are valid for the string given.
	bool is_whitespace(const std::string& str, int start, int end)
	{
		for (int i = start; i <= end; ++i)
		{
			char c = str[i];
			if (c != ' ' && c != '\t') // This should be somewhat similar to the TOKEN_SEPARATOR macro in Scanner.cpp.
			{
				return false;
			}
		}
		return true;
	}

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

	inline void update_precedence(std::string str)
	{
		if (str_to_precedence.count(str))
		{
			OperationPrecedence op = str_to_precedence.at(str);
			if (op > lowop)
			{
				lowop = op;
			}
		}
	}
	
public:
	bool is_malformed = false;

	Scanner()
		:is_interactive(false)
	{
		
	}
	Scanner(bool interact)
		:is_interactive(interact)
	{

	}
	Scanner(uint32_t synline)
		:is_interactive(false)
		,syntactic_linenum(synline)
	{

	}
	~Scanner()
	{
		for(Token* t_ptr : tokens)
		{
			delete t_ptr;
		}
	}

	
	//A wrapper for scan(std::ifstream) that """"casts"""" the fstream into an ifstream. Made for interactive mode.
	void scan(std::fstream& fist, const char* filename)
	{
		std::fstream::fmtflags drapes = fist.flags(); // Does the carpet match?
		fist.close();

		std::ifstream new_handle = std::ifstream(filename); // Did you know fstreams don't keep track of what their filename was? Very strange!

		scan(new_handle);

		fist.open(filename, static_cast<std::ios_base::openmode>(static_cast<int>(drapes))); //VS2019 doesn't make me do this static-casting nonsense but g++ does. :weary:
	}

	//Reads in an istream (most likely an ifstream) line-by-line as an ASCII text file which is supposed to contain Jo�o code.
	void scan(std::istream&);

	//These functions mostly exist to get convenient access to sub-Scanners invoked by the 'include' keyword and its functionality
	std::vector<Token*> get_tokens() const { return tokens; }
	std::vector<OperationPrecedence> get_lowops() const { return lowest_ops; }
	uint32_t get_synline() const { return syntactic_linenum; }

	friend class Parser;
};