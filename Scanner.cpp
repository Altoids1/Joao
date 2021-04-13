
#include "Scanner.h"

#define DIGITS '1','2','3','4','5','6','7','8','9','0'
#define TOKEN_SEPARATOR ' ','\t'



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
			ScannerError(it, "Unterminated string!");
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
	ScannerError(it, "Unterminated string!");
	return it;
}

int Scanner::readNumber(int it)
{
	bool is_double = false; // False until proven otherwise with a decimal place. Note that this means that "1.0" resolves towards a double.
	std::string str = "";
	for (; it < line.length(); ++it)
	{
		char c = line[it];
		switch (c)
		{
		case('.'):
			if (is_double) // Wait, we already found a decimal! What gives??
			{
				ScannerError(it, "Malformed Number!");
				return it;
			}
			is_double = true; // WARNING: CASCADING CASE BLOCK
		case(DIGITS):
			str.push_back(c);
			break;
		case(TOKEN_SEPARATOR,'\n'): // Okay we're done I guess
			makeNumber(is_double, str);
			return it;
		case(';'): // Ugh, we have to do the endline token as well
			makeNumber(is_double, str);
			makeEndline();
			return it;
		}
	}
	makeNumber(is_double, str);
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
			case('"'): // Start of string
				i = readString(i);
				continue;
			case(';'): // End of statement
			{
				makeEndline();
			}
			case(DIGITS):
				i = readNumber(i);
			}
		}
		

		line = "";
	} while (!ifst.eof());
}

#undef DIGITS
#undef TOKEN_SEPARATOR