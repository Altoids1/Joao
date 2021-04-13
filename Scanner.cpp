
#include "Scanner.h"

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
				Token* t = &EndLineToken(linenum);
				linenum++;
				append(t);
			}
			}
		}
		

		line = "";
	} while (!ifst.eof());
}

#undef DIGITS