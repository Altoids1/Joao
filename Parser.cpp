#include "Parser.h"
#include "Directory.h"
#include "Object.h"
#include "ObjectTree.h"
#include "FailureOr.h"
#include "AST.h"
#include "Interpreter.h"

#define SYMBOL_ENUMS(a,b) ((a << 9) | b)



Program Parser::parse() // This Parser is w/o question the hardest part of this to write.
{
	if(tokens.size() == 0) {
		ParserError(nullptr,"Empty programs are not valid!");
		return std::move(t_program);
	}

	std::vector<ClassDefinition*> classdef_list;
	//Step 1. Generate the AST
	for (tokenheader = 0; tokenheader < tokens.size(); ++tokenheader)
	{
		//Expecting: a bunch of classdefs and funcdefs
		//Both of these start with a directory that ends in a '/'Name, so lets read that in
		Token* t = tokens[tokenheader];
		std::string dir_name;
		if (t->class_enum() == Token::cEnum::DirectoryToken)
		{
			dir_name = static_cast<DirectoryToken*>(t)->dir;
			
		}
		else if (t->class_enum() == Token::cEnum::ConstructionToken) // New() override, it seems
		{
			dir_name = static_cast<ConstructionToken*>(t)->dir + "/#constructor";
		}
		else
		{
			ParserError(t, "Unexpected Token at global-scope definition when Directory was expected!");
		}

		

		++tokenheader;
		if(tokenheader >= tokens.size())
			ParserError(t, "Unexpected end of file!");
		//The next token has to be either a '(', which disambiguates us into being a funcdef,
		//or a '{', which brings us towards being a classdef.
		t = tokens[tokenheader];
		if (t->class_enum() != Token::cEnum::PairSymbolToken)
		{
			ParserError(t, "Unexpected Token at global-scope definition!");
			continue; // ?????????
		}

		PairSymbolToken st = *static_cast<PairSymbolToken*>(t); // Allah

		PairSymbolToken::pairOp pop = st.t_pOp;
		if (pop == PairSymbolToken::pairOp::Bracket || !st.is_start)
		{
			ParserError(t, "Unexpected PairSymbol at global-scope definition!");
			continue;
		}
		if (pop == PairSymbolToken::pairOp::Paren) // THIS IS A FUNCDEF! HOT DAMN we're getting somewhere
		{
			int close = find_closing_pairlet(PairSymbolToken::pairOp::Paren, tokenheader + 1);

			std::vector<ImmutableString> pirate_noise;
			if (close != tokenheader + 1)
			{
				pirate_noise.push_back(readName(tokenheader+1)); // Updates tokenheader hopefully
				for (int where = tokenheader; where < close; ++where)
				{
					if (tokens[where]->class_enum() != Token::cEnum::CommaToken)
					{
						ParserError(tokens[where], "Unexpected Token when Comma expected!");
					}
					++where;
					if (where == close)
						ParserError(tokens[where], "Missing parameter name in FunctionDefinition!");
					if (tokens[where]->class_enum() != Token::cEnum::WordToken)
						ParserError(tokens[where], "Unexpected Token when ParameterName expected!");
					pirate_noise.push_back(static_cast<WordToken*>(tokens[where])->word);
				}
			}


			std::vector<Expression*> funcblock = readBlock(BlockType::Function,close+1, static_cast<int>(tokens.size()-1));
			--tokenheader;

			Function* func = new Function(dir_name, std::move(funcblock), std::move(pirate_noise), t->line);

			if (Directory::DotDot(dir_name) == "/") // If this is a classless function in the globalscope
				t_program.set_func(dir_name, func);
			else // This be a method! Avast!
				t_program.set_meth(dir_name, func);

			continue;
		}
		else if (pop == PairSymbolToken::pairOp::Brace) // THIS IS A CLASSDEF!
		{
#ifdef LOUD_TOKENHEADER
			std::cout << "Classdefinition began read at tokenheader " << std::to_string(tokenheader) << std::endl;
#endif
			std::vector<LocalAssignmentStatement*> lasses = readClassDef(tokenheader, static_cast<int>(tokens.size() - 1));

			classdef_list.push_back(new ClassDefinition(dir_name, lasses, t->line));

#ifdef LOUD_TOKENHEADER
			std::cout << "Classdefinition set tokenheader for globalscope to " << std::to_string(tokenheader) << std::endl;
#endif

			--tokenheader; // Handle imminent increment
				
			continue;

			//ParserError(t, "Classdefs are not implemented yet!");
		}
		else
		{
			ParserError(t, "Unexpected Pairsymbol at global-scope definition!");
			continue;
		}
	}
	//Step 2. Generate the object tree
	generate_object_tree(classdef_list);
	for(ClassDefinition* ptr : classdef_list)
		delete ptr;

	//Step 3. Evaluate all constant expressions
	Interpreter parsetime_interp = Interpreter(t_program, false);
	parsetime_interp.push_stack("#init", nullptr); // the Interpreter expects there to always be a topmost stack layer (since usually /main() is there). So here's our own, fake, main.
	for (ConstExpression* ptr : ConstExpression::Registry()) {
		parsetime_interp.push_block();
		ptr->transmute(parsetime_interp);
		parsetime_interp.pop_block();
		if (parsetime_interp.error) {
			parsetime_interp.UncaughtRuntime(parsetime_interp.error);
			t_program.is_malformed = true;
		}
	}
	parsetime_interp.pop_stack();
	

	return std::move(t_program);
}

void Parser::generate_object_tree(std::vector<ClassDefinition*>& cdefs)
{
	/*
	So in general we would like for the properties and methods of object types to be "cooked,"
	insofar that you can get a handle on, an ObjectType during runtime and it'll have all the properties and methods pre-inherited from object types higher in the hierarchy.

	Making this though is a bit of a headache, since the user programmer can declare functions and classdefs anywhere within the script, in any order.
	Further, a type may be poofed into existence by a singular function definition and nowhere else;
	an object type which inherits from its parent in all ways spare that one function.

	Here is my attempt at resolving the problem, written at three in the morning. Enjoy.

	FIXME: While this probably does work, its complexity is practically quadratic, and will have to be reworked to improve its performance later on.
	Additionally, this doesn't allow for correct overloading of the /table class.
	*/

	//Step 0. Define any weird native classes that might exist (so the user may overload them with novel behavior)

	//Step 1. Get a list of all classes that exist in some way
	uncooked_types.merge(t_program.construct_natives()); //Aghast! C++17 be required to run this line!
	std::unordered_set<std::string> list_of_funcs; // Not as used here, mostly for the ParserError

	for (auto it = cdefs.begin(); it != cdefs.end(); ++it) // Ask all the classdefs
	{
		ClassDefinition* cdptr = *it;
		ImmutableString cdstr = cdptr->direct;

		if (uncooked_types.count(cdstr.to_string()))
		{
			ParserError(nullptr, "Duplicate class definition detected!"); // FIXME: Allow for this (with perhaps suppressable warnings regardless)
			continue;
		}

		ObjectType* objtype = new ObjectType(cdstr, cdptr->resolve_properties(*this));

		uncooked_types[cdstr.to_string()] = objtype; // Writing a null to here, I think, still works for creating the entry. Suck it, Lua!
	}
	for (auto it = t_program.definedMethods.begin(); it != t_program.definedMethods.end(); ++it) // Ask all the functions
	{
		Function *const fptr = *it;

		const std::string& function_fullname = fptr->get_name();

		std::string function_shortname = Directory::lastword(function_fullname);

		if (list_of_funcs.count(function_fullname))
		{
			ParserError(nullptr, "Duplicate function definition detected!"); // FIXME: Allow for this (with suppressable warnings anyways)
			continue;
		}
		list_of_funcs.insert(function_fullname);

		std::string dir_f = Directory::DotDot(function_fullname);
		
		if (!uncooked_types.count(dir_f)) // If our type doesn't exist
		{
			ObjectType* objtype = new ObjectType(dir_f);
			objtype->set_typemethod(*this, function_shortname, fptr);
			uncooked_types[dir_f] = objtype; // Make it so!
		}
		else
		{
			uncooked_types[dir_f]->set_typemethod(*this, function_shortname, fptr);
		}

	}

	ObjectTree joao; //Wow, it's the real Jo�o Gaming!
	for (auto it = uncooked_types.begin(); it != uncooked_types.end(); ++it)
	{
		joao.append(it.value());
	}


#ifdef LOUD_AST
	std::cout << "----\nClass Tree:\n";
	joao.dump();
	std::cout << "----\n";
#endif
	
	t_program.definedObjTypes = std::move(uncooked_types); // They're cooked *at this point*, I will note
	
	//Check to make sure that main was defined
	if(!t_program.definedFunctions.lazy_at("main")) {
		ParserError(tokens[0],"/main() function was never defined!");
	}
	return;
}

//Here-there-update; updates through readlvalue and readPower.
ASTNode* Parser::readUnary(int here, int there)
{
	if (tokens[here]->class_enum() != Token::cEnum::SymbolToken)
		return readPower(here, there);

	UnaryExpression::uOps uh = symbol_to_uOp(static_cast<SymbolToken*>(tokens[here]));
	if (uh == UnaryExpression::uOps::NoOp)
	{
		ParserError(tokens[here], "Unexpected symbol when unary operator was expected!");
	}



	return new UnaryExpression(uh, readPower(here+1,there), tokens[here]->line);
	
}

//Vaguely similar and correlated to the behavior of readBinExp's loop, except it collects all lvalues and then builds the exponent chain via iterating in reverse order
//Right-associativity, amirite?
ASTNode* Parser::readPower(int here, int there)
{
	std::vector<ASTNode*> lvalues;

	int where = here;
	int last_power = here - 1;
	for (; where <= there; ++where)
	{
		Token* t = tokens[where];
		switch (t->class_enum())
		{
		case(Token::cEnum::PairSymbolToken): // WARNING: THIS IS A DUMB CTRL+C CTRL+V OF THE ANALOGOUS BLOCK IN READBINEXP(); SHOULD BE IDENTICAL!
		{
			PairSymbolToken pst = *static_cast<PairSymbolToken*>(t);

			if (!pst.is_start)
				ParserError(t, "Unexpected closing pairlet in BinaryExpression!");

			int yonder = find_closing_pairlet(pst.t_pOp, where + 1);
			if (yonder > there) // If down yonder is truly yonder
			{
				ParserError(t, "Could not find closing pairlet for open pairlet in BinaryExpression!");
			}
			where = yonder; // Expecting an imminent increment to make this point the correct place
			continue;
		}
		case(Token::cEnum::EndLineToken):
			goto READPOWER_LEAVE_POWERSEARCH;
		case(Token::cEnum::KeywordToken):
			ParserError(tokens[where], "Unexpected keyword in Expression!");
			continue;
		case(Token::cEnum::SymbolToken):
		{
			//This should just be '^' and nothing else; anything else implies a parsing error in this context.

			SymbolToken* st = static_cast<SymbolToken*>(t);

			if (st->len > 1 || st->get_symbol()[0] != '^') // Hardcoded to '^' for now
			{
				//ParserError(t, "Unexpected symbol when parsing PowerExpression!");
				goto READPOWER_LEAVE_POWERSEARCH;
			}
			lvalues.push_back(readlvalue(last_power + 1, where - 1));
			last_power = where;
			continue;
		}
		default:
			continue;
		}
	}
READPOWER_LEAVE_POWERSEARCH:
	if (lvalues.empty()) // If we didn't find a power
		return readlvalue(here, there); // then this is just a normal lvalue

	// (a ^ (b ^ (c ^ d)))
	
	ASTNode* powertower = readlvalue(last_power + 1, where - 1); // Add the final lvalue of the series of exponentiations, the "c" of a ^ b ^ c
	for (auto it = lvalues.rbegin(); it != lvalues.rend(); ++it)
	{
		powertower = new BinaryExpression(BinaryExpression::bOps::Exponent, *it, powertower, tokens[here]->line);
	}
	return powertower;
}

//lvalue ::= 'null' | 'false' | 'true' | Numeral | LiteralString | tableconstructor | var_access | functioncall |'(' exp ')'
//WARNING: this CAN! return an lvalue in some contexts! (such as during interactive mode, or in any other circumstance where errors aren't fatal)
ASTNode* Parser::readlvalue(int here, int there) // Read an Expression where we know for certain that there's no damned binary operator within it.
{
#ifdef LOUD_TOKENHEADER
	std::cout << "readlvalue starting at " << std::to_string(here) << std::endl;
#endif
	Token* t = tokens[here];

	ASTNode* lvalue = nullptr;
	//First things first, lets find the "lvalue" of this expression, the thing on the left
	switch (t->class_enum())
	{
	case(Token::cEnum::LiteralToken): // 'null' | 'false' | 'true'
	{
		LiteralToken lt = *static_cast<LiteralToken*>(t);
		switch (lt.t_literal)
		{
		case(LiteralToken::Literal::Null):
		{
			lvalue = new Literal(Value());
			break;
		}
		case(LiteralToken::Literal::False):
		{
			lvalue = new Literal(Value(false));
			break;
		}
		case(LiteralToken::Literal::True):
		{
			lvalue = new Literal(Value(true));
			break;
		}
		default:
			ParserError(t, "Unknown LiteralToken type!");
			break;
		}
		tokenheader = here + 1;
#ifdef LOUD_TOKENHEADER
		std::cout << "readlvalue setting tokenheader to " << std::to_string(tokenheader) << std::endl;
#endif
		break;
	}
	case(Token::cEnum::NumberToken): // Numeral
	{
		NumberToken nt = *static_cast<NumberToken*>(t);
		if (nt.is_double)
		{
			lvalue = new Literal(Value(nt.num.as_double));
		}
		else
		{
			lvalue = new Literal(Value(nt.num.as_int));
		}
		tokenheader = here + 1;
#ifdef LOUD_TOKENHEADER
		std::cout << "readlvalue setting tokenheader to " << std::to_string(tokenheader) << std::endl;
#endif
		break;
	}
	case(Token::cEnum::StringToken): // LiteralString
	{
		StringToken st = *static_cast<StringToken*>(t);
		Value val = Value(st.word);
		lvalue = new Literal(val);
		tokenheader = here + 1;
#ifdef LOUD_TOKENHEADER
		std::cout << "readlvalue setting tokenheader to " << std::to_string(tokenheader) << std::endl;
#endif
		break;
	}
	case(Token::cEnum::GrandparentToken):
	case(Token::cEnum::ParentToken):
	case(Token::cEnum::DirectoryToken):
	case(Token::cEnum::WordToken): //functioncall | var_access
	{
		lvalue = readVarAccess(here, there);
		if (tokenheader > there)
		{
			break;
		}
		//Now check to see if this is actually a function call
		//If this fails it's okay; just means that it's a regular var_access, I think
		Token* nexttoken = tokens[tokenheader];
		if (nexttoken->class_enum() == Token::cEnum::PairSymbolToken)
		{
			PairSymbolToken* pst = static_cast<PairSymbolToken*>(nexttoken);
			if (pst->is_start && pst->t_pOp == PairSymbolToken::pairOp::Paren) // I guess it is!
			//functioncall ::= var_access '(' explist ')' [func_access] 
			{
				int close = find_closing_pairlet(PairSymbolToken::pairOp::Paren, tokenheader+1);
				if (close != tokenheader + 1)
					lvalue = new CallExpression(lvalue, readArgs(tokenheader + 1, close - 1), tokens[here]->line);
				else
					lvalue = new CallExpression(lvalue, {},tokens[here]->line);
				//Now check for any func_access
				//(updates tokenheader for us :))
				if (close + 1 >= there)
				{
					tokenheader = close + 1;
					break;
				}
				readFuncAccess(lvalue, close + 1, there);
				break;

			}
		}

		break;
	}
	case(Token::cEnum::EndLineToken):
		ParserError(t, "Endline found when lvalue was expected!");
		break;
	case(Token::cEnum::SymbolToken):
	{
		//This can basically only be an extraneous unary operator.
		//We don't allow multiple unops to happen after each other, at least not yet, so...
		ParserError(t, "Unexpected or underimplemented unary operation!");
		break;
	}
	case(Token::cEnum::ConstructionToken): // directory '/New(' [explist] ')' [func_access]
	{
		ConstructionToken ct = *static_cast<ConstructionToken*>(t);
		consume_paren(true, tokens[static_cast<size_t>(here)+1]);
		int close = find_closing_pairlet(PairSymbolToken::pairOp::Paren, here + 2);
		if (close != here + 2)
			lvalue = new Construction(ct.dir, readArgs(here + 2, close - 1), tokens[here]->line);
		else
			lvalue = new Construction(ct.dir, {}, tokens[here]->line);

		if (close + 1 >= there)
		{
			tokenheader = close + 1;
			break;
		}
		readFuncAccess(lvalue, close + 1, there);
		break;
	}
	case(Token::cEnum::PairSymbolToken): // tableconstructor | '(' exp ')'
	{
		const PairSymbolToken& pst = *static_cast<PairSymbolToken*>(t);
		switch (pst.t_pOp)
		{
		default:
			ParserError(t, "Unexpected or underimplemented use of PairSymbolToken!");
			tokenheader = here + 1;
			lvalue = new Literal(Value());
			break;
		case(PairSymbolToken::pairOp::Brace):	// tableconstructor ::= '{' [exp] {',' exp} [',']'}' | '{'[Name '=' exp]{ ','[Name '=' exp] }[','] '}'
		{
			consume_open_brace(here);
			tokenheader = here + 1;
			//Try to read in the key-value version and fallback to the value-array one if it fails
			Token* trytoken = tokens[tokenheader];
			if (trytoken->class_enum() == Token::cEnum::WordToken || trytoken->class_enum() == Token::cEnum::StringToken)
			{
				//Not incrementing tokenheader yet since this is a bit of a peak-ahead
				if (tokens[tokenheader + 1]->class_enum() == Token::cEnum::SymbolToken)
				{
					SymbolToken* st = static_cast<SymbolToken*>(tokens[tokenheader + 1]);
					if (symbol_to_aOp(st) == AssignmentStatement::aOps::Assign)
					{
						//This is definitely Key-value pairs!
						//'{'[Name '=' exp]{ ','[Name '=' exp] }[','] '}'
						/*
						FIXME:
						Right now, this specific way to initialize a table has a snowflake ASTNode type to handle its execution.
						In the future, it would probably be best to have this just pass into the normal /table constructor,
						with each key-value pair being a Value of type Tuple (a thing that does not exist yet in this language)
						*/
						HashTable<std::string,ASTNode*> nodes;
						int yonder = find_closing_pairlet(PairSymbolToken::pairOp::Brace, tokenheader + 2);
						do
						{
							Token* nametoken = tokens[tokenheader];
							std::string namestr;
							if (nametoken->class_enum() == Token::cEnum::WordToken)
							{
								namestr = static_cast<WordToken*>(nametoken)->word.to_string();
							}
							else if (nametoken->class_enum() == Token::cEnum::StringToken)
							{
								namestr = static_cast<StringToken*>(nametoken)->word;
							}
							else
							{
								ParserError(nametoken, "Unexpected token when table key expected in table constructor!");
								break; // Out of while(true)
							}
							++tokenheader;
							//Consume an assign symbol
							if (tokens[tokenheader]->class_enum() != Token::cEnum::SymbolToken || // Either not a symbol at all or
								symbol_to_aOp(static_cast<SymbolToken*>(tokens[tokenheader])) != AssignmentStatement::aOps::Assign) // not '='
							{
								ParserError(tokens[tokenheader], "Unexpected token when '=' symbol expected in table constructor!");
								break;
							}
							++tokenheader;
							int valueyonder = find_comma(tokenheader, yonder - 1); // Where the value expression ought to end
							ASTNode* valuenode;
							if (!valueyonder) // Failed to find comma
							{
								valuenode = readExp(tokenheader, yonder - 1);
							}
							else
							{
								valuenode = readExp(tokenheader, valueyonder - 1);
								++tokenheader; // Consume the comma
							}
							nodes[namestr] = valuenode;
						} while (tokenheader < yonder);
						lvalue = new BaseTableConstruction(std::move(nodes), tokens[tokenheader]->line);
						tokenheader = yonder + 1;
						break; // Out of case(PairSymbolToken::pairOp::Brace)
					}
				}
			}
			//Value array!
			//'{' [exp] {',' exp}[',']'}'
			int yonder = find_closing_pairlet(PairSymbolToken::pairOp::Brace, tokenheader);
			if (yonder == tokenheader) // Special case where it's a blank init; "{}"
			{
				lvalue = new Construction("/table", {}, tokens[tokenheader]->line);
				++tokenheader;
				break;
			}
			std::vector<ASTNode*> nodes;
			do
			{
				int valueyonder = find_comma(tokenheader, yonder - 1); // Where the value expression ought to end
				if (!valueyonder) // Failed to find comma
				{
					nodes.push_back(readExp(tokenheader, yonder - 1));
				}
				else
				{
					nodes.push_back(readExp(tokenheader, valueyonder - 1));
					++tokenheader; // Consume the comma
				}
			} while (tokenheader < yonder);
			lvalue = new Construction("/table", nodes, tokens[tokenheader]->line);
			tokenheader = yonder + 1; // Make sure we consume the ending brace
			break;
		}
		case(PairSymbolToken::pairOp::Paren): // '(' exp ')'
			consume_paren(true, t);
			int close = find_closing_pairlet(PairSymbolToken::pairOp::Paren, here+1);
			
			lvalue = readExp(here + 1, close - 1); // ReadExp will increment tokenheader for us, hopefully.

			consume_paren(false); // Consumes that paren we found
		}
		break;
	}
	case(Token::cEnum::KeywordToken):
	{
		const KeywordToken& key_token = *static_cast<KeywordToken*>(t);
		if (key_token.t_key == KeywordToken::Key::Const) {// 'const{' [block] '}'
			if(is_interactive) {
				ParserError(t,"Const blocks are not available in interactive mode.");
			}
			lvalue = new ConstExpression(readBlock(BlockType::Function, here + 1, there)); // readBlock just handles all the busy work for us here :)
		}
		break;
	}
	default:
		ParserError(t, "Unexpected Token when reading lvalue!");
		break;
	}

	if (!lvalue)
	{
		ParserError(t, "Failed to comprehend lvalue of Expression!");
		lvalue = new Literal(Value());
	}

	
	return lvalue;
}

// Reads, from here to there, scanning for BinaryExpressions of its OperationPrecedence and lower
ASTNode* Parser::readBinExp(Scanner::OperationPrecedence op, int here, int there) 
{
#ifdef LOUD_TOKENHEADER
	std::cout << "readBinExp(" << Scanner::precedence_tostring(op) << ") starting at " << std::to_string(here) << std::endl;
#endif
	/*
	if (expect_close_paren)
		std::cout << "I expect close paren!\n";
	else
		std::cout << "I don't expect close paren!\n";
	*/
	//std::cout << "Starting to search for operation " << Scanner::precedence_tostring(op) << "...\n";

	if (op == Scanner::OperationPrecedence::Unary) // If we're at the near-bottom, evaluate a unary
	{
		return readUnary(here, there); // Hear-hear!
	}
	else if (op == Scanner::OperationPrecedence::Power)
	{
		return readPower(here, there);
	}


	ASTNode* lhs = nullptr;

	int where = here;

	//Now lets try to read the binary operation in question
	for (; where <= there; ++where)
	{
		Token* t2 = tokens[where];
		switch (t2->class_enum())
		{
		case(Token::cEnum::PairSymbolToken): // So the 'exp' grammarthingie doesn't really... care? about these pairsymbols?
		{//So what we are to do, is leave them for readlvalue to pick up and think about. All we need to do is find our operators and do the recursive splitting around them.
			PairSymbolToken pst = *static_cast<PairSymbolToken*>(t2);
			
			if(!pst.is_start)
				ParserError(t2, "Unexpected closing pairlet in BinaryExpression!");

			int yonder = find_closing_pairlet(pst.t_pOp, where + 1);
			if (yonder > there) // If down yonder is truly yonder
			{
				ParserError(t2, "Could not find closing pairlet for open pairlet in BinaryExpression!");
			}
			where = yonder; // Expecting an imminent increment to make this point the correct place
			continue;
		}
		case(Token::cEnum::EndLineToken):
			//++where; // Because all semicolons are explicitly consumed thru consume_semicolon(), hopefully, BinExp must leave pointing to the semicolon it found, if it did find one
			goto READBOP_LEAVE_BOPSEARCH;
		case(Token::cEnum::KeywordToken):
			ParserError(tokens[where], "Unexpected keyword in Expression!");
			continue;
		case(Token::cEnum::SymbolToken):
		{
			BinaryExpression::bOps boopitybeep = readbOp(static_cast<SymbolToken*>(t2));

			if (boopitybeep == BinaryExpression::bOps::NoOp)
			{
				continue;
			}
			else if (bOp_to_precedence.at(boopitybeep) == op) // WE GOT A HIT!
			{
				
				//ALL THIS ASSUMES LEFT-ASSOCIATIVITY, AS IN ((1 + 2) + 3) + 4

				/*
				So in some instances it's necessary to disambiguate whether a symbol is meant as a unary operation or a binary operation, such as with the minus sign (-).
				The way that this is disambiguated is by determining if the symbol is at occurring at the very start of the expression, or immediately after another, different symbol;
				"-A" and
				"A+-B", for example.
				readBinExp() doesn't really know or care about the distinction between these two things, but there does need to be the following special case to detect for a unary,
				which is written generically in case other ambiguous unary/binary symbols are added to the language besides the minus symbol.
				*/

				if (here == where) // If this symbol occurred at the VERY start of this expression
				{
					//Then it's actually a unary operation I think
					lhs = readUnary(here, there);
					where = tokenheader - 1;
					continue;
				}

				if (!lhs)
				{
					lhs = readBinExp(static_cast<Scanner::OperationPrecedence>(static_cast<uint8_t>(op) - 1), here, where-1);
				}

				ASTNode* right = readBinExp(static_cast<Scanner::OperationPrecedence>(static_cast<uint8_t>(op) - 1), where+1, there);

				if(lhs && lhs->class_name() == "Literal" && right && right->class_name() == "Literal") // Really primitive const-rolling
				{
					auto literalLHS = static_cast<Literal*>(lhs);
					auto literalRHS = static_cast<Literal*>(right);
					FailureOr ret = BinaryExpression::BinaryOperation(literalLHS->const_resolve(*this,true),literalRHS->const_resolve(*this,true),boopitybeep);
					//Don't need those old literals anymore!
					delete literalLHS;
					delete literalRHS;
					if(ret.didError)
					{
						ParserError(t2,std::get<FailureOr::Failure>(ret.data).what.to_string());
						lhs = new Literal(Value());
					}
					else
					{
						lhs = new Literal(std::get<Value>(ret.data));
					}
				} 
				else
					lhs = new BinaryExpression(boopitybeep, lhs, right, t2->line);
				
				continue;
			}
			else if (bOp_to_precedence.at(boopitybeep) > op)
			{
				goto READBOP_LEAVE_BOPSEARCH;
			}

			//we don't got a hit. :(
			continue;
		}
		default:
			continue;
		}
	}
READBOP_LEAVE_BOPSEARCH:
	//std::cout << "Exiting search for " << Scanner::precedence_tostring(op) << " at token " << where << "...\n";
#ifdef LOUD_TOKENHEADER
	std::cout << "readBinExp(" << Scanner::precedence_tostring(op) << ") setting tokenheader to " << std::to_string(where) << std::endl;
#endif
	tokenheader = where; // FIXME: I don't even really know what exactly there is to fix here, just know that readBinExp does some funky bullshit with the tokenheader that may cause it to malpoint in anything readBinExp calls
	if (lhs)
		return lhs;

	//ParserError(tokens[here], "readBinExp failed to read binary expression!");


	//If we're here then it seems the operation(s) we're looking for doesn't exist
	//So lets just return... whatever it is in the lower stacks
	lhs = readBinExp(static_cast<Scanner::OperationPrecedence>(static_cast<uint8_t>(op) - 1), here, there);


	
	return lhs;
	
}

ASTNode* Parser::readExp(int here, int there)
{
#ifdef LOUD_TOKENHEADER
	std::cout << "Expression starts at " << std::to_string(here) << std::endl;
#endif

	//Okay, god, we're really getting close to actually being able to *resolve* something.

	Token* t = tokens[here];

	Scanner::OperationPrecedence lowop = lowest_ops[t->syntactic_line];

	if (lowop == Scanner::OperationPrecedence::NONE) // If we know for a fact that no binary operation takes place on this syntactic line
	{
		//++tokenheader;
#ifdef LOUD_TOKENHEADER
		ASTNode* ans = readlvalue(here, there);
		std::cout << "Expression leaves at " << std::to_string(tokenheader) << std::endl;
		return ans;
#else
		return readlvalue(here, there);
#endif
	}

	//we know (or at least kinda think) that there's a binop afoot.
#ifdef LOUD_TOKENHEADER
	ASTNode* ans = readBinExp(lowop, here, there);
	std::cout << "Expression leaves at " << std::to_string(tokenheader) << std::endl;
	return ans;
#else
	return readBinExp(lowop, here, there);
#endif
	
}

//Here-there-update; updates through ReadExp().
LocalAssignmentStatement* Parser::readLocalAssignment(int here, int there) // Value x = 3;
{
	Token* t = tokens[here];
	if (t->class_enum() != Token::cEnum::LocalTypeToken)
		ParserError(t, "Unexpected Token where LocalTypeToken was expected!");

	LocalType tuh;

	switch (static_cast<LocalTypeToken*>(t)->t_type) // I feel like a lvl 10 Wizard when I write lines of C++ like this
	{
	case(LocalType::Value):
	case(LocalType::Boolean):
	case(LocalType::Number):
	case(LocalType::Object):
	case(LocalType::String):
		tuh = static_cast<LocalTypeToken*>(t)->t_type;
		break;
	case(LocalType::Local):
		ParserError(t, "'local' is a reserved word!");
		return nullptr;
	default:
		ParserError(t, "Underimplemented LocalTypeToken detected!");
		return nullptr;
	}

	Identifier* id = new Identifier(readName(here+1));

	AssignmentStatement::aOps aesop = readaOp(here+2, false);

	if (aesop == AssignmentStatement::aOps::NoOp)
	{
		if(tokens[here+2]->class_enum() != Token::cEnum::EndLineToken)
			ParserError(t, "Unexpected token when reading LocalAssignment!");
		//This implies something like "Value x;", which ought to be equivalent to "Value x = null;"
		//So... lets make it equivalent!
		
		//(We don't consume the semicolon; it's assumed by higher stacks that *they* will be the ones to consume it)
		//(We know it's there, though ;) )
		return new LocalAssignmentStatement(id, new Literal(Value()), AssignmentStatement::aOps::Assign, tuh, tokens[here+1]->line);
	}

	ASTNode* rvalue = readExp(here+3,there);

	return new LocalAssignmentStatement(id, rvalue, aesop, tuh, tokens[here+1]->line);
}

//Consumes the open and close braces for you.
std::vector<Expression*> Parser::readBlock(BlockType bt, int here, int there) // Tokenheader state should point to the opening brace of this block.
{
	//Blocks are composed of a starting brace, following by statements, and are ended by an end brace.

	//Starting brace checks
	Token* t = tokens[here];
	consume_open_brace(here);

	//Now the fun begins!

	//In AST land, blocks are a list of expressions associated with a particular scope.
	std::vector<Expression*> allStatements;
	//I miss the pre-REPL era where statement parsing was unrolled into this thing.
	for (int where = here+1; where <= there; where = tokenheader)
	{
		t = tokens[where];
		switch(t->class_enum()) {
			case(Token::cEnum::EndLineToken):
				ParserError(t, "Unexpected semicolon in block!"); // Yes I'm *that* picky, piss off
				continue;
			case(Token::cEnum::PairSymbolToken): {
				PairSymbolToken pt = *static_cast<PairSymbolToken*>(t);
				if (pt.t_pOp == PairSymbolToken::pairOp::Brace && !pt.is_start)
				{
					//This pretty much has to be the end of the block; lets return our vector of shit.
					tokenheader = where + 1;
					goto BLOCK_RETURN_ASTS; // Can't break because we're in a switch in a for-loop :(
				}
				ParserError(t, "Unexpected PairSymbol while traversing block!");
				break;
			}
			case(Token::cEnum::StringToken):
			case(Token::cEnum::NumberToken):
				ParserError(t, "Misplaced literal detected while traversing block!");
				break;
			default: LIKELY
				break;
		}
		Expression* statement = readStatement(bt, where, there);
		if(statement != nullptr)
			allStatements.push_back(statement);
	}
	ParserError(t, "Unable to find ending brace of block!");

BLOCK_RETURN_ASTS:
	if (allStatements.size() == 0)
	{
		ParserError(t, "Block created with no Expressions inside!");
	}

#ifdef LOUD_TOKENHEADER
	std::cout << "Exiting block with header pointed at " << std::to_string(tokenheader) << ".\n";
#endif
	return allStatements;
}

Expression* Parser::readStatement(BlockType bt, int& where, int there) {
	Token* t = tokens[where];
	switch (t->class_enum())
	{
	case(Token::cEnum::EndLineToken): UNLIKELY // This is a null statement, then. Weird.
		return nullptr;
	case(Token::cEnum::KeywordToken): // Statements started by a keyword
	{
		KeywordToken kt = *static_cast<KeywordToken*>(t);
		switch (kt.t_key)
		{
		case(KeywordToken::Key::For): // For loops!
		{
			++where; tokenheader = where;// Move the header past the for keyword
			consume_paren(true); // (
			int yonder = find_closing_pairlet(PairSymbolToken::pairOp::Paren, tokenheader);

			size_t semicolon = find_first_semicolon(tokenheader, yonder-1); // Using "find" instead of "get" here,
			//since we not sure whether this is a generic for-loop or a for-each.

			if (!semicolon) // Can't be a generic for-loop, then. Attempt to read foreach
			{
				WordToken* keytoken, *valtoken; // This syntax is so borked
				if (tokens[tokenheader]->class_enum() != Token::cEnum::WordToken) //keytoken
				{
					ParserError(tokens[tokenheader], "Unexpected token when Word expected while reading for-each!");
				}
				keytoken = static_cast<WordToken*>(tokens[tokenheader]);
				++tokenheader;
				if (tokens[tokenheader]->class_enum() != Token::cEnum::CommaToken) // The comma
				{
					ParserError(tokens[tokenheader], "Unexpected token when Comma expected while reading for-each!");
				}
				++tokenheader;
				if (tokens[tokenheader]->class_enum() != Token::cEnum::WordToken) //valtoken
				{
					ParserError(tokens[tokenheader], "Unexpected token when Word expected while reading for-each!");
				}
				valtoken = static_cast<WordToken*>(tokens[tokenheader]);
				++tokenheader;
				if (tokens[tokenheader]->class_enum() != Token::cEnum::KeywordToken
					|| static_cast<KeywordToken*>(tokens[tokenheader])->t_key != KeywordToken::Key::In) //"in" keyword
				{
					ParserError(tokens[tokenheader], "Unexpected token when 'in' keyword expected while reading for-each!");
				}
				++tokenheader;
				ASTNode* table_node = readExp(tokenheader, yonder - 1);
				//assert(yonder == tokenheader);
				consume_paren(false); // )

				return new ForEachBlock(keytoken->word, valtoken->word, table_node, readBlock(BlockType::For, tokenheader, there));
			}

			ASTNode* init;
			if (find_aOp(tokenheader, static_cast<int>(semicolon)))
			{
				if (tokens[tokenheader]->class_enum() == Token::cEnum::LocalTypeToken)
					init = readLocalAssignment(tokenheader, static_cast<int>(semicolon));
				else
					init = readAssignmentStatement(tokenheader, static_cast<int>(semicolon));
			}
			else
			{
				init = readExp(tokenheader, static_cast<int>(semicolon));
			}

			where = static_cast<int>(semicolon + 1);
			semicolon = get_first_semicolon(where, yonder-1);
			ASTNode* cond = readExp(where, static_cast<int>(semicolon)); // Assignments do not evaluate to anything in Jo�o so putting one in a conditional is silly
			where = static_cast<int>(semicolon + 1);
			ASTNode* inc;
			if (find_aOp(where, yonder-1))
			{
				inc = readAssignmentStatement(where, yonder-1);
			}
			else
			{
				inc = readExp(where, yonder - 1);
			}
				
			consume_paren(false); // )

			std::vector<Expression*> for_block = readBlock(BlockType::For,tokenheader,there); // { ...block... }
			return new ForBlock(init, cond, inc, for_block);
		}
		case(KeywordToken::Key::If):
		{
			++where; tokenheader = where;
			consume_paren(true); // (
			int yonder = find_closing_pairlet(PairSymbolToken::pairOp::Paren, tokenheader);
			ASTNode* cond = readExp(tokenheader, yonder-1);
			consume_paren(false); // )

			std::vector<Expression*> if_block = readBlock(BlockType::If,tokenheader,there);

			IfBlock* ifStatement = new IfBlock(cond, if_block, tokens[where]->line);
			
			while (tokens[tokenheader]->class_enum() == Token::cEnum::KeywordToken)
			{
				KeywordToken* ktptr = static_cast<KeywordToken*>(tokens[tokenheader]);

				ASTNode* elif_cond = nullptr;
				std::vector<Expression*> elif_block = {};

				int block_start = tokenheader + 1;

				bool was_else = true;

				switch (ktptr->t_key)
				{
				default:
					goto IF_BLOCK_LEAVE_ELIF_SEARCH;
				case(KeywordToken::Key::Elseif):
				{
					was_else = false;
					++tokenheader;
					consume_paren(true); // (
					int yonder = find_closing_pairlet(PairSymbolToken::pairOp::Paren, tokenheader);
					elif_cond = readExp(tokenheader, yonder - 1);
					consume_paren(false); // )
					block_start = tokenheader;
					[[fallthrough]]; //ROLLS INTO THE ELSE CODE
				}
				case(KeywordToken::Key::Else):
				{
					elif_block = readBlock(BlockType::If, block_start, there);
					ifStatement->append_else(new IfBlock(elif_cond, elif_block, ktptr->line));
				}
				}

				if (was_else) // Stops the Parser from accepting multiple else blocks, or an elif after an else, or whatever
					break;
			}
			IF_BLOCK_LEAVE_ELIF_SEARCH:
			return ifStatement;
		}
		case(KeywordToken::Key::Elseif):
			ParserError(t, "Unexpected Elseif keyword with no corresponding If!");
			break;
		case(KeywordToken::Key::Else):
			ParserError(t, "Unexpected Else keyword with no corresponding If!");
			break;
		case(KeywordToken::Key::Break):
		{
			if (bt == BlockType::Function)
				ParserError(t, "Unexpected Break statement in Function block!");
			++where; tokenheader = where; // Consume break token
			Token* t2 = tokens[where];

			int brk = 1;

			switch (t2->class_enum())
			{
			case(Token::cEnum::NumberToken):
			{
				NumberToken* nt = static_cast<NumberToken*>(t2);
				if (nt->is_double)
				{
					if(nt->num.as_double != int64_t(nt->num.as_double))
						ParserError(t, "Unexpected double-type literal after Break keyword; 'break' may only take expressionless integer literals as input!");
					brk = nt->num.as_double;
				}
				else
				{
					brk = nt->num.as_int;
				}

				if (brk < 1) // No "break 0;" please, thanks
				{
					ParserError(t, "Non-positive integer given as argument to Break statement!");
				}

				++where; tokenheader = where; // Consume Number token
				consume_semicolon();
				break;
			}//Rolls over into EndLineToken case
			case(Token::cEnum::EndLineToken):
				consume_semicolon();
				break;
			default:
				ParserError(t, "Unexpected Token after Break keyword; 'break' may only take expressionless integer literals as input!");
				break;
			}
			return new BreakStatement(brk);
		}
		case(KeywordToken::Key::Continue):
		{
			if (bt == BlockType::Function)
				ParserError(t, "Unexpected continue statement in Function block!");
			++where; tokenheader = where; // Consume continue token
			consume_semicolon();
			return new ContinueStatement(); // no more lol :(
		}
		case(KeywordToken::Key::While):
		{				
			++where; tokenheader = where;
			consume_paren(true); // (
			int yonder = find_closing_pairlet(PairSymbolToken::pairOp::Paren, tokenheader);
			ASTNode* cond = readExp(tokenheader, yonder-1);
			consume_paren(false); // )

			std::vector<Expression*> while_block = readBlock(BlockType::While,tokenheader,there);
			return new WhileBlock(cond, while_block, tokens[where]->line);
		}
		case(KeywordToken::Key::Try):
		{
			++where; tokenheader = where; // Consume this try token
			std::vector<Expression*> try_block = readBlock(BlockType::Try, tokenheader, there);
			Token* _catch = tokens[tokenheader];
			if (_catch->class_enum() != Token::cEnum::KeywordToken || static_cast<KeywordToken*>(_catch)->t_key != KeywordToken::Key::Catch)
			{
				ParserError(_catch, "Unexpected token when 'catch' block was expected!");
			}
			++tokenheader;
			consume_paren(true); // (
			ImmutableString err_name = readName(tokenheader);
			consume_paren(false); // )
			std::vector<Expression*> catch_block = readBlock(BlockType::Catch, tokenheader, there);
			return new TryBlock(try_block, err_name, catch_block);
		}
		case(KeywordToken::Key::Return):
		{
			++where; tokenheader = where; // Consume this return token
			/*
			Return can either have an expression, or no expression, in which case it returns null.
			*/
			ReturnStatement* returnStatement;
			if (tokens[tokenheader]->class_enum() == Token::cEnum::EndLineToken) // Return null
			{
				returnStatement = new ReturnStatement(new Literal(Value()), tokens[where]->line);
			}
			else // Return stuff
			{
				returnStatement = new ReturnStatement(readExp(where, there - 1), tokens[where]->line);
				
			}
			
			consume_semicolon();
			return returnStatement;
		}
		case(KeywordToken::Key::Catch):
		{
			ParserError(t, "'catch' keyword found with no associated 'try' block!");
			break;
		}
		case(KeywordToken::Key::Throw):
		{
			++where; tokenheader = where;
			/*
			There's two possibilities for throw:
			1. There's no error object operand and we're supposed to return a generic /error object.
			2. There is an expression which we (the Parser) will assume somehow resolves towards being an /error object.
			
			Lets test for both.
			*/
			// #1
			if (tokens[tokenheader]->class_enum() == Token::cEnum::EndLineToken) 
			{
				consume_semicolon();
				return new ThrowStatement(nullptr); // we can't create a default /error object yet because the object tree has yet to be generated,
				//so lets just have the interpreter handle it :)
			}
			// #2
			int yonder = get_first_semicolon(where + 1, there);
			auto exp = readExp(where, yonder-1);
			consume_semicolon();
			return new ThrowStatement(exp);
		}
		default:
			ParserError(t, "Unknown keyword!");
			break;
		}
		break;
	}
	case(Token::cEnum::PairSymbolToken):
	{
		ParserError(t, "Unexpected PairSymbol while traversing expression!");
		break;
	}
	case(Token::cEnum::SymbolToken):
	{
		ParserError(t, "Unexpected or underimplemented Symbol while traversing expression!"); //FIXME: Allow for unary expressions to be their own statements (important for ++ and -- when they are implemented)
		break;
	}
	case(Token::cEnum::WordToken):
	case(Token::cEnum::DirectoryToken):
	case(Token::cEnum::ParentToken):
	//If the Grammar serves me right, this is either a varstat or a functioncall.
	//The main way to disambiguate is to check if the var_access is if any assignment operation takes place on this line.
	{
		int yonder = static_cast<int>(get_first_semicolon(where+1, there));
		int found_aop = find_aOp(where + 1, yonder - 1);

		if(found_aop) { // varstat
			auto ret = readAssignmentStatement(where, yonder);
			consume_semicolon();
			return ret;
		}
		// otherwise, functioncall
		ASTNode* luh = readlvalue(where, yonder - 1); // defers to readlvalue(), since it contains the functioncall constructor within it
		consume_semicolon();
		if (luh->is_expression())
			return static_cast<Expression*>(luh);
		ParserError(t, "Unexpected " + luh->class_name() +  " expression when Statement was expected!");
		break;
	}
	case(Token::cEnum::LocalTypeToken): // So this implies we're about to read in an initialization.
	{
		//std::cout << "Before: " << std::to_string(tokenheader) << std::endl ;
		LocalAssignmentStatement* localassign = readLocalAssignment(where, there);
		consume_semicolon();
		if(localassign) // if not nullptr
			return localassign;
		[[fallthrough]]; // I THINK???
	}
	case(Token::cEnum::StringToken):
	case(Token::cEnum::NumberToken):
		ParserError(t, "Misplaced literal detected while traversing expression!");
		break;
	default: UNLIKELY
		ParserError(t, "Unknown Token type found when traversing expression!");
		break;
	}
	return nullptr;
}

std::vector<LocalAssignmentStatement*> Parser::readClassDef(int here, int there)
{
	Token* t = tokens[here];
	consume_open_brace(here);

	std::vector<LocalAssignmentStatement*> ASTs;

	//This grammar state is pretty simple: It's an endless list of LocalAssignStatement-compatible declarations that will later be read to form the object tree/hash.
	int where = here + 1;
	for (; where <= there; ++where)
	{
		t = tokens[where];
		switch (t->class_enum())
		// readLocalAssignment() does this check too, but doing it here lets me give a more specific ParserError. Parser errors aren't errors; they're merely failing with *style*!
		{
		case(Token::cEnum::LocalTypeToken):
		{
			//LocalTypeToken* ltt_ptr = static_cast<LocalTypeToken*>(t);
			LocalAssignmentStatement* lassy = readLocalAssignment(where, there);

			ASTs.push_back(lassy);
			consume_semicolon();
			where = tokenheader - 1;
			break;
		}
		case(Token::cEnum::PairSymbolToken):
		{
			PairSymbolToken pt = *static_cast<PairSymbolToken*>(t);
			if (pt.t_pOp == PairSymbolToken::pairOp::Brace && !pt.is_start)
			{
				//This pretty much has to be the end of the block; lets return our vector of shit.
				tokenheader = where + 1;
				//std::cout << "Hello!\n";
				goto READ_CLASSDEF_RETURN_ASTS; // Can't break because we're in a switch in a for-loop :(
			}
			break;
		}
		default:
			ParserError(t, "Unexpected or underimplemented Token detected while traversing class definition!");
		}
	}
	ParserError(t, "Unable to find ending brace of block!");

READ_CLASSDEF_RETURN_ASTS:
	if (ASTs.size() == 0)
	{
		ParserError(t, "Classdef created with no Expressions inside!");
	}
#ifdef LOUD_TOKENHEADER
	std::cout << "Exiting Classdef with header pointed at " << std::to_string(tokenheader) << ".\n";
#endif
	return ASTs;
}

Function* Parser::try_parse_function() {
	//FIXME: This is *slightly* repeated work from parse(), but I think that's okay.
	tokenheader = 0;
	Token* t = tokens[tokenheader];
	std::string dir_name;
	if (t->class_enum() == Token::cEnum::DirectoryToken)
	{
		dir_name = static_cast<DirectoryToken*>(t)->dir;
		
	}
	tokenheader++;
	if(tokens.size() >= tokenheader)
		return nullptr;
	if(tokens[tokenheader]->class_enum() != Token::cEnum::PairSymbolToken) {
		return nullptr;
	}
	PairSymbolToken st = *static_cast<PairSymbolToken*>(tokens[tokenheader]);
	switch(st.t_pOp) {
		case PairSymbolToken::pairOp::Brace:
			//TODO: Implement this situation!
			ParserError(tokens[tokenheader],"Instantiating classes within interactive mode is not yet available!");
			break;
		case PairSymbolToken::pairOp::Bracket:
			ParserError(tokens[tokenheader],"Unexpected brace in function declaration!");
			break;
		default:
			break;
	}
	int close = find_closing_pairlet(PairSymbolToken::pairOp::Paren, tokenheader + 1);
	std::vector<ImmutableString> pirate_noise;
	if (close != tokenheader + 1)
	{
		pirate_noise.push_back(readName(tokenheader+1)); // Updates tokenheader hopefully
		for (int where = tokenheader; where < close; ++where)
		{
			if (tokens[where]->class_enum() != Token::cEnum::CommaToken)
			{
				ParserError(tokens[where], "Unexpected Token when Comma expected!");
			}
			++where;
			if (where == close)
				ParserError(tokens[where], "Missing parameter name in FunctionDefinition!");
			if (tokens[where]->class_enum() != Token::cEnum::WordToken)
				ParserError(tokens[where], "Unexpected Token when ParameterName expected!");
			pirate_noise.push_back(static_cast<WordToken*>(tokens[where])->word);
		}
	}

	std::vector<Expression*> funcblock = readBlock(BlockType::Function,close+1, static_cast<int>(tokens.size()-1));
	--tokenheader;

	Function* func = new Function(dir_name, std::move(funcblock), std::move(pirate_noise), t->line);
	return func;
}

//Determines whether the entire token stream should grammatically constitute a statement (instead of an expression)
//Used for disambiguating between the two in the REPL code.
bool Parser::is_statement() {
	//What distinguishes a statement from an expression, in João,
	//is that it contains at least one of the following tokens: if, while, for, try, break, throw, continue, return, or an assignment op.
	for(Token* t : tokens) {
		switch (t->class_enum())
		{
		case Token::cEnum::KeywordToken: {
			KeywordToken* keywordToken = static_cast<KeywordToken*>(t);
			switch(keywordToken->t_key) {
				case KeywordToken::Key::If:
				case KeywordToken::Key::Break:
				case KeywordToken::Key::For:
				case KeywordToken::Key::Continue:
				case KeywordToken::Key::Return:
				case KeywordToken::Key::Throw:
				case KeywordToken::Key::Try:
					return true;
				default:
					continue;
			}
			break;
		}
		case Token::cEnum::SymbolToken: {
			SymbolToken* symbolToken = static_cast<SymbolToken*>(t);
			if(symbol_to_aOp(symbolToken) != AssignmentStatement::aOps::NoOp) {
				return true;
			}
		}
		default:
			break;
		}
	}
	return false;
}

//This pointer has to be cleaned up by the caller when they're done with it.
ASTNode* Parser::parse_repl_expression(Program& realProgram) {
	// repl ::= {funcdef | stat | exp | nothing}
	if(tokens.empty())
		return nullptr;
	//check if it's a function definition, first, since that's the really weird option
	if(Function* func = try_parse_function(); func != nullptr) {
		if (Directory::DotDot(func->get_name()) == "/") // If this is a classless function in the globalscope
			realProgram.set_func(func->get_name(), func);
		else // This be a method! Avast! 
			//TODO: Make this work!
			ParserError(tokens[0],"Defining methods of classes within interactive mode is not yet implemented!");
		return nullptr; // Nothing to execute yet :)
	}
	//If it isn't a function, then it's a statement or a stray expression.
	ASTNode* ret;
	if(is_statement()) {
		int where = 0;
		ret = readStatement(BlockType::Function, where, tokens.size()-1);
	} else {
		ret = readBinExp(Scanner::OperationPrecedence::Logical,0,tokens.size()-1);
	}
	if(t_program.is_malformed)
		return nullptr;
#ifdef LOUD_AST
	std::cout << "----\nExpression AST:\n";
	std::cout << ret->dump(0);
	std::cout << "----\n";
#endif
	return ret;
}

void Parser::IncludeAlienType(ObjectType* ot)
{
	uncooked_types[ot->get_name()] = ot;
}