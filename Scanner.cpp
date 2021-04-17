
#include "Scanner.h"

#define DIGITS '1','2','3','4','5','6','7','8','9','0'
#define HEX_DIGITS DIGITS,'a','b','c','d','e','f'
#define TOKEN_SEPARATOR ' ','\t','\n'

//PairSymbols are things like [ and ], { and }, etc.; things that must come in pairs.
#define PAIRSYMBOL '{','}','[',']','(',')'

#define SYMBOL '+','-','*','/','.',',','&','|','^','~','?','>','<','=','!','%'
#define DOUBLEABLE_SYMBOL '+','-','&','|','^','='

//Defines used to mark what is a valid Word (incl. valid variable names)
#define ascii_UPPER 'A','B','C','D','E','F','G','H','I','J','K','L','M','N','O','P','Q','R','S','T','U','V','W','X','Y','Z'
#define ascii_lower 'a','b','c','d','e','f','g','h','i','j','k','l','m','n','o','p','q','r','s','t','u','v','w','x','y','z'
#define ascii_other '_'

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
			case('n', 'N'):
				str.push_back('\n');
			case('t', 'T'):
				str.push_back('\t');
			default:
				str.push_back(cc); // \" and \' and all that rubbish funnels into here in the end.
				break; // Breaks out of the switch, not the for-loop, Allah-willing.
			}
		}
		case('"')://Delimiter found, returning...
		{
			Token* t = &StringToken(linenum,str);
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
		case(DIGITS):
			str.push_back(c);
			break;
		case(TOKEN_SEPARATOR): // Okay we're done I guess
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
	Token* t = &PairSymbolToken(linenum, line[it]);
	append(t);
	return it;
}

int Scanner::readSymbol(int it)
{
	char first = line[it];
	char second = '\0';

	//We don't really have to do a whole proper for-loop here since symbols can only be one or two characters in size.
	if (it < line.length())
	{
		char c = line[it + 1]; // Do a little look-ahead
		switch (c)
		{
		case(DOUBLEABLE_SYMBOL): // if this is a doubleable symbol
			if (c == first)// and it genuinely doubles
			{
				second = c; //woag it's a double symbol
				++it;
				break;
			}
		}
	}

	Token* t = &SymbolToken(linenum, first, second);
	append(t);
	return it;
}

int Scanner::readWord(int it)
{
	std::string str = "";
	for (; it < line.length(); ++it)
	{
		char c = line[it];
		switch (c)
		{
		case(ascii_lower,ascii_other,ascii_UPPER):
			str.push_back(c);
		case(TOKEN_SEPARATOR): // Oohp, we're done
			makeWord(str);
			return it;
		case('"'): // Oy, you can't just be starting strings right after Word things!
			std::cout << "What?\n\n\n";
			ScannerError(it, ScanError::MalformedString);
			return it;
		default: // we're not smart enough to smell any other rubbish, just make the Word and leave if we get to this point.
			makeWord(str);
			return --it;
		}
	}
	makeWord(str);
	return it;
}

void Scanner::scan(std::ifstream& ifst)
{
	do
	{
		std::getline(ifst, line);

		for (int i = 0; i < line.length(); ++i)
		{
			char c = line[i];
			switch (c)
			{
			case(TOKEN_SEPARATOR):
				continue;
			case('"'): // Start of string
				i = readString(i);
				continue;
			case(';'): // End of statement
			{
				makeEndline();
				continue;
			}
			case(DIGITS):
				i = readNumber(i);
				continue;
			case(SYMBOL):
				i = readSymbol(i);
				continue;
			case(PAIRSYMBOL):
				i = readPairSymbol(i);
			case(ascii_UPPER,ascii_lower,ascii_other):
				i = readWord(i);
				continue;
			default:
				ScannerError(i, ScanError::UnknownCharacter);
			}
		}
		

		line = "";
	} while (!ifst.eof());
}

#undef DIGITS
#undef HEX_DIGITS
#undef TOKEN_SEPARATOR
#undef SYMBOL
#undef DOUBLEABLE_SYMBOL 