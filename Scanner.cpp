
#include "Scanner.h"

#define DIGITS case '1':case '2':case '3':case '4':case '5':case '6':case '7':case '8':case '9':case '0'
#define HEX_DIGITS DIGITS,case 'a':case 'b':case 'c':case 'd':case 'e':case 'f'
#define TOKEN_SEPARATOR case ' ':case '\t':case '\n'

//PairSymbols are things like [ and ], { and }, etc.; things that must come in pairs.
#define PAIRSYMBOL case '{':case '}':case '[':case ']':case '(':case ')'

//SYMBOL on its own does not include '/' due to its special and oft-ambiguous nature. It is often still a symbol though, it's just that Scanner treats it special.
#define SYMBOL case '+':case '-':case '*':case '.':case ',':case '&':case '|':case '^':case '~':case '?':case '%':case '#'

//These are symbols which can (possibly) begin an equality operator.
#define EQ_SYMBOL case '>':case '<':case '=':case '!'

#define DOUBLEABLE_SYMBOL case '+':case '-':case '&':case '|':case '^':case '=':case '>':case '<':case '#':case '/':case '.':case '~'

//Defines used to mark what is a valid Word (incl. valid variable names)
#define ascii_UPPER case 'A':case 'B':case 'C':case 'D':case 'E':case 'F':case 'G':case 'H':case 'I':case 'J':case 'K':case 'L':case 'M':case 'N':case 'O':case 'P':case 'Q':case 'R':case 'S':case 'T':case 'U':case 'V':case 'W':case 'X':case 'Y':case 'Z'
#define ascii_lower case 'a':case 'b':case 'c':case 'd':case 'e':case 'f':case 'g':case 'h':case 'i':case 'j':case 'k':case 'l':case 'm':case 'n':case 'o':case 'p':case 'q':case 'r':case 's':case 't':case 'u':case 'v':case 'w':case 'x':case 'y':case 'z'
#define ascii_other case '_'




std::string Scanner::getString(int& it)
{
	/*
	This function assumes that line[it] is the '"' character.
	*/
	++it;
	std::string str = "";
	for (; it < line.length(); ++it)
	{
		char c = line[it];
		switch (c)
		{
		case('\n'):
			ScannerError(it, ScanError::UnterminatedString);
			return str;
		case('\\'): // An escape character! Fancy
		{
			char cc = line[++it];
			switch (cc)
			{
			case('n'):
			case('N'):
				str.push_back('\n');
				break;
			case('t'):
			case('T'):
				str.push_back('\t');
				break;
			default:
				str.push_back(cc); // \" and \' and all that rubbish funnels into here in the end.
				break; // Breaks out of the switch, not the for-loop, Allah-willing.
			}
			break;
		}
		case('"')://Delimiter found, returning...
		{
			return str;
		}
		default:
			str.push_back(c);
		}
	}
	ScannerError(it, ScanError::UnterminatedString);
	return str;
}

int Scanner::readString(int it)
{
	/*
	This function assumes that line[it] is the '"' character.
	*/
	Token* t = new StringToken(linenum, syntactic_linenum, getString(it));
	append(t);
	return it;
}

int Scanner::readNumber(int it)
{
	bool is_double = false; // False until proven otherwise with a decimal place. Note that this means that "1.0" resolves towards a double.
	int base = 10; // TODO: Support other bases
	std::string str = "";
	for (; it < line.length(); ++it)
	{
		char c = line[it];
		switch (c)
		{
		case('_'): // Helper underline that makes it easier to read the number, i.e "1_000_000"
		// TODO: Support ' character once internationali[sz]ation is working
			break; // just consume it and move on
		case('.'):
			if (is_double) // Wait, we already found a decimal! What gives??
			{
				ScannerError(it, ScanError::MalformedNumber);
				return it;
			}
			is_double = true;
			[[fallthrough]]; // WARNING: CASCADING CASE BLOCK
		DIGITS:
			str.push_back(c);
			break;
		TOKEN_SEPARATOR: // Okay we're done I guess
			makeNumber(it, is_double, str, base);
			return it;
		case(';'): // Ugh, we have to do the endline token as well
			makeNumber(it, is_double, str, base);
			makeEndline();
			return it;
		default: //Found something wacky?
			//just make our number and move on.
			makeNumber(it, is_double, str, base);
			return --it;
		}
	}
	makeNumber(it, is_double, str, base);
	return it;
}

int Scanner::readPairSymbol(int it)
{
	Token* t = new PairSymbolToken(linenum, syntactic_linenum, line[it]);
	append(t);
	return it;
}

/*
This function handles symbols which are EQ_SYMBOLs, ones which can (but may possibly not) be the beginning of an equality operation.
These are separated out to reduce the load on (and complexity of) readSymbol(), as well as allowing for more descriptive errors for this sort of symbol
for instance, being able to warn about swapping <= to become =< instead, an invalid symbol which would get a more confusing error with an alternative implementation.

Still generates a SymbolToken, just is a bit nicer about it.
*/
int Scanner::readEqSymbol(int it, std::istream& ifst)
{
	char first = line[it];
	const bool can_be_two = static_cast<size_t>(it + 1) < line.length();
	switch (first)
	{
	case('='):
		if (can_be_two)
		{
			char second = line[static_cast<size_t>(it + 1)];
			if (second == '=')
			{
				append(new SymbolToken(linenum, syntactic_linenum, first, second));
				update_precedence(std::string{ first, second });
				return it + 1;
			}
			else if (second == '<' || second == '>' || second == '!') // Ah, they got the order wrong. Tut tut!
			{
				ScannerError(it+1, ScanError::SwappedEqOp);
			}
		}
		append(new SymbolToken(linenum, syntactic_linenum, first));
		update_precedence(std::string{ first});
		return it;
	case('!'):
	case('<'):
	case('>'):
		if (can_be_two)
		{
			char second = line[it + 1];
			if (second == first || second == '=')
			{
				append(new SymbolToken(linenum, syntactic_linenum, first, second));
				update_precedence(std::string{ first, second });
				return it + 1;
			}
			//else, falls through into the outer scope's returning of the one-char operator
		}
		append(new SymbolToken(linenum, syntactic_linenum, first));
		update_precedence(std::string{ first });
		return it;
	default:
		ScannerError(it, ScanError::Unknown);
		return it;
	}
}

int Scanner::readSymbol(int it, std::istream& ifst)
{
	char first = line[it];
	char second = '\0';

	//We don't really have to do a whole proper for-loop here since symbols can only be one or two characters in size.
	if (static_cast<size_t>(it + 1) < line.length())
	{
		char c = line[static_cast<size_t>(it + 1)]; // Do a little look-ahead
		switch (c)
		{
		DOUBLEABLE_SYMBOL: // if this is a doubleable symbol
			//Parent and grandparent access-operators
			if (first == '.')
			{
				if (c == '/')
				{
					if (static_cast<size_t>(it + 2) < line.length())
					{
						//Charlie because it's the third one & it's a char. I'm very funny.
						char charlie = line[it + 2];
						switch (charlie)
						{
						ascii_lower:
						ascii_UPPER:
						ascii_other:
							append(new ParentToken(linenum, syntactic_linenum));
							return it + 1;
						default:
							ScannerError(it + 2, ScanError::BadDaddy);
							break;
						}
					}
					else
					{
						ScannerError(it + 1, ScanError::BadDaddy);
					}
				}
				else if (c == '.')
				{
					if (static_cast<size_t>(it + 2) < line.length() && line[static_cast<size_t>(it+2)] == '/' && static_cast<size_t>(it + 3) < line.length()) // If this all fails then it just assumes it's a concat op and rolls into the higher-scope if statement below
					{
						char dorothy = line[static_cast<size_t>(it + 3)]; // We're peeking WAY fucking ahead aren't we
						switch (dorothy)
						{
						ascii_lower:
						ascii_UPPER:
						ascii_other:
						case('.'): //FIXME: Listen Jim, I'm a Scanner, not a smart & recursive algorithm doctor!
							append(new GrandparentToken(linenum, syntactic_linenum));
							return it + 2;
						default:
							ScannerError(it + 3, ScanError::BadGrandpa);
							break;
						}
					}
				}
			}
			//Double-char operators and things; '..' rolls over into here if finding '../' fails
			if (c == first || c == '=')
			{
				second = c; //woag it's a double symbol
				if (c == '#') // If ##, meaning a linecomment
				{
					return readComment(it+2,ifst);
				}
				++it;
				update_precedence(std::string{ first, second });
				append(new SymbolToken(linenum, syntactic_linenum, first, second));
				return it;
			}
			//Casually rolls-over into the default case when it realizes this isn't a two-char symbol
			break;
		DIGITS:
			if(first == '.') // Oh this is a quirky number that starts with the decimal point. ".123" or whatever.
				return readNumber(it);
			[[fallthrough]];
		default:
			break;
		}
	}
	Token* t;
	if (first == '.')
	{
		t = new MemberToken(linenum, syntactic_linenum);
	}
	else if (first == ',')
	{
		t = new CommaToken(linenum, syntactic_linenum);
	}
	else
	{
		update_precedence(std::string{ first });
		t = new SymbolToken(linenum, syntactic_linenum, first, second);
	}


	

	append(t);
	return it;
}

std::string Scanner::getWord(int& it)
{
	/*
	This function assumes that the first character, the character pointed to by it,
	is an acceptable initializing character for a word (A-Za-z_ or whatever)
	*/

	//std::cout << "Starting getWord at " << std::to_string(it) << std::endl;
	std::string str = { line[it] };
	for (++it; it < line.length(); ++it)
	{
		char c = line[it];
		switch (c)
		{
		ascii_lower:
		ascii_other:
		ascii_UPPER:
		DIGITS:
			str.push_back(c);
			continue;
		TOKEN_SEPARATOR: // Oohp, we're done
			++it;
			return str;
		case('"'): // Oy, you can't just be starting strings right after Word things!
			ScannerError(it, ScanError::MalformedString);
			++it;
			return str;
		default: // we're not smart enough to smell any other rubbish, just make the Word and leave if we get to this point.
			//std::cout << "Returning getWord with it at " << std::to_string(it) << std::endl;
			return str;
		}
	}
	return str;
}

int Scanner::readWord(int it)
{
	makeWord(getWord(it));
	--it;
	return it;
}

//it here points to the 3rd character of comment, including the hashes. If it's '#' then it's a longcomment (or an attempt at one)
int Scanner::readComment(int it,std::istream& ifst)
{

	if(it > line.length() || line[it] != '#') // If this is a linecomment
		return static_cast<int>(line.length()); // Skip to end of line

	//Else, we are now in the readlongcomment zone

	if (it != 2 && !is_whitespace(line,0,it-3))
		ScannerError(it, ScanError::MalformedLongComment);

	do
	{
		std::getline(ifst, line);

		if (line.substr(0, 3) == "###") // Found the closing longcomment
		{
			return static_cast<int>(line.length());
		}
		size_t index = line.find("###");
		if (index != std::string::npos)
		{
			if (index + 3 < line.length() && !is_whitespace(line, index + 3, line.length() - 1))
			{
				ScannerError(index, ScanError::MalformedLongComment);
			}
			return index + 3;
		}
	} while (!ifst.eof());
	ScannerError(0, ScanError::UnterminatedLongComment);
	return INT32_MAX; // Should be unreachable but uhhh.. whatever
}


//Reads in a '/' character and disambiguates it between meaning a DirectoryToken, ConstructionToken, or a SymbolToken.
int Scanner::readSlash(int it, std::istream& ifst)
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
	++it;
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
			//std::cout << "Starting to read word of dir at " << std::to_string(it) << std::endl;
			std::string str = getWord(it); // Silently updates $it by reference
			//std::cout << "Word it got was " << str << std::endl;

			if (keywordhash.count(str) || typehash.count(str) || literalhash.count(str)) // Can't be a keyword
			{
				ScannerError(it, ScanError::MalformedDirectory);
			}
			if (str == "New") // Poor man's keyword
			{
				dir.pop_back(); // Gets rid of the '/' we got earlier
				append(new ConstructionToken(linenum, syntactic_linenum, dir));
				--it;
				return it;
			}
			if (str == "")
			{
				ScannerError(it, ScanError::MalformedDirectory);
			}

			dir.append(str);
			--it; // Decrement to help against incoming increment
			continue;
		}
		else
		{
			//std::cout << "Starting to read slash of dir at " << std::to_string(it) << std::endl;
			switch (line[it])
			{
			case('/'):
				dir.push_back('/');
				continue;
			TOKEN_SEPARATOR:
			PAIRSYMBOL:
			SYMBOL:
			EQ_SYMBOL:
			case(';'): // End of statement; can mean we're in an expression like "return /global_var_thing;"
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
	if (dir == "/")
	{
		Token* t = new SymbolToken(linenum, syntactic_linenum, '/');
		append(t);
		return it;
	}
	if (readword)
		ScannerError(it, ScanError::MalformedDirectory);

	Token* t = new DirectoryToken(linenum, syntactic_linenum, dir);
	append(t);
	return it;
}

void Scanner::scan(std::istream& ifst)
{
	do
	{
		linenum++; // At the start so that we're 1-indexed instead of 0-indexed
		std::getline(ifst, line);

		//include checking
		if (line.size() > 10 && line.substr(0, 9) == "include \"")
		{
			int start = 10;
			std::string strfile = getString(start);

			std::ifstream includedfile;
#ifdef _DEBUG
			std::cout << "Opening file " << strfile << "...\n";
#endif
			includedfile.open(strfile);
			if (!includedfile.good())
			{
				Terminal::SetColor(std::cerr,Terminal::Color::Red);
				std::cerr << "Unable to open file " << strfile << "!\n";
				Terminal::SetColor(std::cerr,Terminal::Color::RESET);
				exit(1);
			}
			
			Scanner subscanner(syntactic_linenum+1);
			subscanner.scan(includedfile);

			//Token merging
			std::vector<Token*> their_tokens = subscanner.get_tokens();
			tokens.insert(tokens.end(), their_tokens.begin(), their_tokens.end());

			//Precedence merging
			std::vector<OperationPrecedence> their_ops = subscanner.get_lowops();
			lowest_ops.insert(lowest_ops.end(), their_ops.begin(), their_ops.end());

			syntactic_linenum = subscanner.get_synline() + 1;
			lowest_ops.push_back(lowop);
			lowop = OperationPrecedence::NONE;

			continue;
		}

		for (size_t i = 0; i < line.length(); ++i)
		{
			
			char c = line[i];

			switch (c)
			{
			TOKEN_SEPARATOR:
				continue;
			case('"'): // Start of string
				i = readString(static_cast<int>(i));
				continue;
			case(';'): // End of statement
			{
				
				makeEndline();
				continue;
			}
			DIGITS:
				i = readNumber(static_cast<int>(i));
				continue;
			case('/'):
				i = readSlash(static_cast<int>(i), ifst);
				continue;
			EQ_SYMBOL:
				i = readEqSymbol(static_cast<int>(i), ifst);
				continue;
			SYMBOL:
				i = readSymbol(static_cast<int>(i), ifst); // the 2nd argument is strange but bear with me here
				continue;
			PAIRSYMBOL:
				i = readPairSymbol(static_cast<int>(i));
				continue;
			ascii_UPPER:
			ascii_lower:
			ascii_other:
				i = readWord(static_cast<int>(i));
				continue;
			default:
				ScannerError(static_cast<unsigned int>(i), ScanError::UnknownCharacter);
			}
		}
		
		//Update Scanner's properties
		line = "";
		

	} while (!ifst.eof());
	lowest_ops.push_back(lowop);
	lowop = OperationPrecedence::NONE;

#ifdef LOUD_SCANNER
	std::cout << "SCANNER_DEBUG: Contents of Tokens:\n";
	for (size_t i = 0; i < tokens.size(); ++i)
	{
		std::cout << "TOKEN#: "<< i << "\t" << tokens[i]->dump() << std::endl;
	}

	std::cout << "OperatorPrecedence per syntax line:\n";
	for (size_t i = 0; i < lowest_ops.size(); ++i)
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
#undef EQ_SYMBOL