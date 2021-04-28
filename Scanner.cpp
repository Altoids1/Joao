
#include "Scanner.h"

#define DIGITS case '1':case '2':case '3':case '4':case '5':case '6':case '7':case '8':case '9':case '0'
#define HEX_DIGITS DIGITS,case 'a':case 'b':case 'c':case 'd':case 'e':case 'f'
#define TOKEN_SEPARATOR case ' ':case '\t':case '\n'

//PairSymbols are things like [ and ], { and }, etc.; things that must come in pairs.
#define PAIRSYMBOL case '{':case '}':case '[':case ']':case '(':case ')'

//SYMBOL on its own does not include '/' due to its special and oft-ambiguous nature. It is often still a symbol though, it's just that Scanner treats it special.
#define SYMBOL case '+':case '-':case '*':case '.':case ',':case '&':case '|':case '^':case '~':case '?':case '>':case '<':case '=':case '!':case '%':case '#'

#define DOUBLEABLE_SYMBOL case '+':case '-':case '&':case '|':case '^':case '=':case '>':case '<':case '#':case '/'

//Defines used to mark what is a valid Word (incl. valid variable names)
#define ascii_UPPER case 'A':case 'B':case 'C':case 'D':case 'E':case 'F':case 'G':case 'H':case 'I':case 'J':case 'K':case 'L':case 'M':case 'N':case 'O':case 'P':case 'Q':case 'R':case 'S':case 'T':case 'U':case 'V':case 'W':case 'X':case 'Y':case 'Z'
#define ascii_lower case 'a':case 'b':case 'c':case 'd':case 'e':case 'f':case 'g':case 'h':case 'i':case 'j':case 'k':case 'l':case 'm':case 'n':case 'o':case 'p':case 'q':case 'r':case 's':case 't':case 'u':case 'v':case 'w':case 'x':case 'y':case 'z'
#define ascii_other case '_'



int Scanner::readString(int it)
{
	++it;
	std::string str = "";
	for (; it < line.length(); ++it)
	{
		char c = line[it];
		switch (c)
		{
		case('\n'):
			ScannerError(it, ScanError::UnterminatedString);
			return it;
		case('\\'): // An escape character! Fancy
		{
			char cc = line[++it];
			switch (cc)
			{
			case('n'):
			case('N'):
				str.push_back('\n');
			case('t'):
			case('T'):
				str.push_back('\t');
			default:
				str.push_back(cc); // \" and \' and all that rubbish funnels into here in the end.
				break; // Breaks out of the switch, not the for-loop, Allah-willing.
			}
		}
		case('"')://Delimiter found, returning...
		{
			Token* t = new StringToken(linenum, syntactic_linenum,str);
			append(t);
			return it;
		}
		default:
			str.push_back(c);
		}
	}
	ScannerError(it, ScanError::UnterminatedString);
	return it;
}

int Scanner::readNumber(int it)
{
	bool is_double = false; // False until proven otherwise with a decimal place. Note that this means that "1.0" resolves towards a double.
	int base = 10;
	std::string str = "";
	for (; it < line.length(); ++it)
	{
		char c = line[it];
		switch (c)
		{
		case('_'): // Helper underline that makes it easier to read the number, i.e "1_000_000"
			break; // just consume it and move on
		case('.'):
			if (is_double) // Wait, we already found a decimal! What gives??
			{
				ScannerError(it, ScanError::MalformedNumber);
				return it;
			}
			is_double = true; // WARNING: CASCADING CASE BLOCK
		DIGITS:
			str.push_back(c);
			break;
		TOKEN_SEPARATOR: // Okay we're done I guess
			makeNumber(is_double, str, base);
			return it;
		case(';'): // Ugh, we have to do the endline token as well
			makeNumber(is_double, str, base);
			makeEndline();
			return it;
		default: //Found something wacky?
			//just make our number and move on.
			makeNumber(is_double, str, base);
			return --it;
		}
	}
	makeNumber(is_double, str, base);
	return it;
}

int Scanner::readPairSymbol(int it)
{
	Token* t = new PairSymbolToken(linenum, syntactic_linenum, line[it]);
	append(t);
	return it;
}

int Scanner::readSymbol(int it, std::ifstream& ifst)
{
	char first = line[it];
	char second = '\0';

	//We don't really have to do a whole proper for-loop here since symbols can only be one or two characters in size.
	if (it + 1 < line.length())
	{
		char c = line[it + 1]; // Do a little look-ahead
		switch (c)
		{
		DOUBLEABLE_SYMBOL: // if this is a doubleable symbol
			if (c == first)// and it genuinely doubles
			{
				second = c; //woag it's a double symbol
				if (c == '#') // If ##, meaning a linecomment
				{
					return readComment(it+2,ifst);
				}
				++it;
				std::string str{ first, second };
				if (str_to_precedence.count(str))
				{
					OperationPrecedence op = str_to_precedence.at(str);
					if (op > lowop)
					{
						lowop = op;
					}
				}
				break;
			}
		default:
			std::string str{first};
			if (str_to_precedence.count(str))
			{
				OperationPrecedence op = str_to_precedence.at(str);
				if (op > lowop)
				{
					lowop = op;
				}
			}
		}
	}

	Token* t = new SymbolToken(linenum, syntactic_linenum, first, second);

	append(t);
	return it;
}

std::string Scanner::getWord(int& it)
{
	std::string str = "";
	for (; it < line.length(); ++it)
	{
		char c = line[it];
		switch (c)
		{
		ascii_lower:
		ascii_other:
		ascii_UPPER:
			str.push_back(c);
			continue;
		TOKEN_SEPARATOR: // Oohp, we're done
			return str;
		case('"'): // Oy, you can't just be starting strings right after Word things!
			ScannerError(it, ScanError::MalformedString);
			return str;
		default: // we're not smart enough to smell any other rubbish, just make the Word and leave if we get to this point.
			--it;
			return str;
		}
	}
}

int Scanner::readWord(int it)
{
	makeWord(getWord(it));
	return it;
}

int Scanner::readComment(int it,std::ifstream& ifst)
{

	if(it > line.length() || line[it] != '#') // If this is a linecomment
		return line.length(); // Skip to end of line

	//Else, we are now in the readlongcomment zone

	if (it != 2)
		ScannerError(it, ScanError::MalformedLongComment);

	do
	{
		std::getline(ifst, line);

		if (line.substr(0, 3) == "###") // Found the closing longcomment
		{
			break;
		}

	} while (!ifst.eof());
	return line.length();
}


//Reads in a '/' character and disambiguates it between meaning a DirectoryToken, ConstructionToken, or a SymbolToken.
int Scanner::readSlash(int it, std::ifstream& ifst)
{
/*
Situations that prove that this is an operator: (easier)
1. There's a space after it, or a tab or newline
2. there's a second slash (indicating the floor division operator "//")

Situations that prove this is a directory: (harder)
1. The next token is a Word

Situations that prove the programmer is an idiot:
1. None of the above conditions end up being true
*/
	if (it + 1 == line.length())
		return readSymbol(it, ifst);
	switch (line[static_cast<size_t>(it + 1)])
	{
	case('/'):
	TOKEN_SEPARATOR:
		return readSymbol(it, ifst);
	default:
		break;
	}

	std::string dir = "/";
	bool readword = true;
	for (; it < line.length(); ++it, readword = !readword)
	{
		/*
		If reading a word, we expect:
		- A word
		- "New"
		If not reading a word, we expect:
		- A '/'
		- TOKEN_SEPARATOR
		- Any symbol or pairsymbol, really
		*/
		if (readword)
		{
			std::string str = getWord(it); // Silently updates $it by reference
			if (keywordhash.count(str) || typehash.count(str) || literalhash.count(str)) // Can't be a keyword
			{
				ScannerError(it, ScanError::MalformedDirectory);
			}
			if (str == "New") // Poor man's keyword
			{
				str.pop_back(); // Deletes the slash we got earlier
				append(new ConstructionToken(linenum, syntactic_linenum, str));
				return it;
			}

			dir.append(str);
			continue;
		}
		else
		{
			switch (line[it])
			{
			case('/'):
				dir.push_back('/');
				continue;
			TOKEN_SEPARATOR:
			PAIRSYMBOL:
			SYMBOL:
				--it; // Marks that whoever called us ought to start reading at this char, the $it we're looking at right now
				goto READSLASH_FINISH;
				break;
			default:
				ScannerError(it, ScanError::MalformedDirectory);
				break;
			}
		}
	}
	READSLASH_FINISH:
	if (readword)
		ScannerError(it, ScanError::MalformedDirectory);

	Token* t = new DirectoryToken(linenum, syntactic_linenum, dir);
	append(t);
	return it;
}

void Scanner::scan(std::ifstream& ifst)
{
	do
	{
		linenum++; // At the start so that we're 1-indexed instead of 0-indexed
		std::getline(ifst, line);

		for (int i = 0; i < line.length(); ++i)
		{
			
			char c = line[i];

			switch (c)
			{
			TOKEN_SEPARATOR:
				continue;
			case('"'): // Start of string
				i = readString(i);
				continue;
			case(';'): // End of statement
			{
				
				makeEndline();
				continue;
			}
			DIGITS:
				i = readNumber(i);
				continue;
			case('/'):
				i = readSlash(i, ifst);
				continue;
			SYMBOL:
				i = readSymbol(i, ifst); // the 2nd argument is strange but bear with me here
				continue;
			PAIRSYMBOL:
				i = readPairSymbol(i);
				continue;
			ascii_UPPER:
			ascii_lower:
			ascii_other:
				i = readWord(i);
				continue;
			default:
				ScannerError(i, ScanError::UnknownCharacter);
			}
		}
		
		//Update Scanner's properties
		line = "";
		

	} while (!ifst.eof());
	lowest_ops.push_back(lowop);
	lowop = OperationPrecedence::NONE;

#ifdef LOUD_SCANNER
	std::cout << "SCANNER_DEBUG: Contents of Tokens:\n";
	for (int i = 0; i < tokens.size(); ++i)
	{
		std::cout << "TOKEN#: "<< i << "\t" << tokens[i]->dump() << std::endl;
	}

	std::cout << "OperatorPrecedence per syntax line:\n";
	for (int i = 0; i < lowest_ops.size(); ++i)
	{
		std::string str = precedence_tostring(lowest_ops[i]);
		std::cout << "LINE: " << i << " OP: " << str << std::endl;
	}

#endif
}

#undef DIGITS
#undef HEX_DIGITS
#undef TOKEN_SEPARATOR
#undef SYMBOL
#undef DOUBLEABLE_SYMBOL 