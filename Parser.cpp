#include "Parser.h"

#define SYMBOL_ENUMS(a,b) ((a << 9) | b)

void Program::construct_natives()
{
	//TEXT MANIPULATION
	definedFunctions["print"] = &NativeFunction("print", [](std::vector<Value> args)
	{
		if (args.size())
		{
			std::cout << args[0].to_string();
			for (int i = 1; i < args.size(); ++i)
			{
				std::cout << '\t' << args[i].to_string();
			}
		}
		std::cout << "\n";
		return Value();
	});
	definedFunctions["tostring"] = &NativeFunction("tostring", [](std::vector<Value> args)
	{
		return Value(args[0].to_string());
	});
	definedFunctions["void_stellakafuhparenthessisluaunderscorestatewithacapitalscommaluaunderscoreallocfvoidstarud"] = &NativeFunction("void_stellakafuhparenthessisluaunderscorestatewithacapitalscommaluaunderscoreallocfvoidstarud", [](std::vector<Value> args)
	{
		return Value(7);
	});

	//MATHEMATICS
	definedFunctions["sqrt"] = &NativeFunction("sqrt", [](std::vector<Value> args)
	{
		Value arg = args[0];
		switch (arg.t_vType)
		{
		case(Value::vType::Double):
			return Value(sqrt(arg.t_value.as_double));
		case(Value::vType::Integer):
			return Value(sqrt(arg.t_value.as_int));
		case(Value::vType::Bool):
			return Value(sqrt(arg.t_value.as_bool));
		default:
			return Value(Value::vType::Null, int(ErrorCode::BadArgType));
		}
	});
	definedFunctions["sin"] = &NativeFunction("sin", [](std::vector<Value> args)
	{
		Value arg = args[0];
		switch (arg.t_vType)
		{
		case(Value::vType::Double):
			return Value(sin(arg.t_value.as_double));
		case(Value::vType::Integer):
			return Value(sin(arg.t_value.as_int));
		case(Value::vType::Bool):
			return Value(sin(arg.t_value.as_bool));
		default:
			return Value(Value::vType::Null, int(ErrorCode::BadArgType));
		}
	});
	definedFunctions["cos"] = &NativeFunction("cos", [](std::vector<Value> args)
	{
		Value arg = args[0];
		switch (arg.t_vType)
		{
		case(Value::vType::Double):
			return Value(cos(arg.t_value.as_double));
		case(Value::vType::Integer):
			return Value(cos(arg.t_value.as_int));
		case(Value::vType::Bool):
			return Value(cos(arg.t_value.as_bool));
		default:
			return Value(Value::vType::Null, int(ErrorCode::BadArgType));
		}
	});
}


Program Parser::parse()
{
	assert(tokens.size() > 0);

	//*MAN*, how do you even write something like this? Uh...
	Token* prevtoken = nullptr;
	for (auto it = tokens.begin(); it != tokens.end(); ++it)
	{
		Token* t = *it;
		switch (t->class_enum())
		{
		case(Token::cEnum::SymbolToken): // FINDING A SYMBOL
		{
			SymbolToken st = *(static_cast<SymbolToken*>(t)); // I pray to Allah every sundown, asking him to ensure that doing this shit actually works
			char* c = st.get_symbol();
			uint16_t switcher = SYMBOL_ENUMS(c[0],c[1]);
			switch (switcher)
			{
			case(SYMBOL_ENUMS('/','\0')):
				//POSSIBLE MEANINGS: DIVISION, DIRECTORY, ...
				/*
				So a thing that can be used to disambiguate between divison and directory is uh,
				the fact that directories start with a beginning slash,
				which can be interpreted as being sort of a, "unary operation" against the initial word symbol, maybe
				*/
				switch (t_expect)
				{
				case(Expect::Anything):
					//ROLLS OVER INTO DIRSYMBOL
				case(Expect::DirSymbol):
					t_expect = Expect::DirName;
					//Yeah, good. OK.
					expression_stack.push_back(t);
					continue;
				case(Expect::Operator):
					expression_stack.push_back(t); // I dunno.
					continue;
				default:
					ParserError(t, "Unknown syntax around '/' operator!");

				}
			default:
				ParserError(t, "Unknown Symbol!");
			}
		}
		case(Token::cEnum::WordToken): //FINDING A WORD
			switch (t_expect)
			{
			case(Expect::Anything): // This is like, the start of an expression or something I guess
			case(Expect::LiteralOrVariable):
				expression_stack.push_back(t);
				t_expect = Expect::Operator;
				continue;
			case(Expect::DirName):
				expression_stack.push_back(t);
				t_expect = Expect::DirSymbol;
			default:
				ParserError(t, "Unknown syntax around Word!");
			}
		case(Token::cEnum::EndLineToken): // FINDING A SEMICOLON
			//flush_expression_stack();
			continue;
		case(Token::cEnum::PairSymbolToken): // FINDING A PAIRLET
		{
			PairSymbolToken pst = *(static_cast<PairSymbolToken*>(t));
			uint8_t b = pst.is_start, c = uint8_t(pst.t_pOp);
			/*
			switch (t_expect)
			{
			case(Expect::StartOfBlock):

			}
			*/
		}
		case(Token::cEnum::KeywordToken):
		{
			KeywordToken kt = *(static_cast<KeywordToken*>(t)); // Allah
			switch (kt.t_key)
			{
			case(KeywordToken::Key::Return):

			default:
				ParserError(t, "Keyword recognised by Scanner but not by Parser!");
			}
		}
		default:
			ParserError(t, "Unknown Token Type!");
		}
		prevtoken = t;
	}

	//do stuff
	return t_program;
}