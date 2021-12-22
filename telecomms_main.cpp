/*
This file is supposed to handle being the front-end entry point for Joao as a telecommunications language
for the video game Space Station 13.
*/


#include "Forward.h"
#include "AST.h"
#include "Interpreter.h"
#include "Object.h"
#include "Parser.h"
#include "Args.h"
#include <chrono>

#include "shared_enums.cpp.rs"

constexpr uint8_t PACKET_VERSION = 0; // The version number for the sort of telecomm packets being transferred
//between this process and the Auxtools one.
//Stored as a uint8_t in the packets so can't exceed 255 :woozy:




template <typename Ty_>
bool has(const std::vector<Ty_>& v, const Ty_& value)
{
	for (Ty_ thing : v)
	{
		if (thing == value)
		{
			return true;
		}
	}
	return false;
}

Object* makeSignal(Interpreter& interp, const char* filename, ObjectType& sigtype)
{
	//So I get that this is called a FIFO and all
	//But if the Auxtools layer is doing this correctly, the packet should just come in the correct order as-written
	//In one, big, fat packet that we can just read immediately

	/*
	VERSION
	SOURCE
	FREQ
	JOB
	CONTENT
	*/
	std::ifstream t(filename);
	std::string line;
	std::getline(t,line);
	if(static_cast<uint8_t>(line.at(0)) != PACKET_VERSION) // Has to be the EXACT SAME version.
	{
		dumpError(filename,"Packet is wrong version!",joaoErrorCode::TransferError);
		exit(joaoErrorCode::TransferError);
	}

	Object* signal = sigtype.makeObject(interp,{});

	//SOURCE
	std::getline(t,line);
	signal->set_property_raw("source",Value(line));
	//FREQ
	std::getline(t,line);
	signal->set_property_raw("freq",Value(static_cast<int>(line.at(0))));
	//JOB
	std::getline(t,line);
	signal->set_property_raw("job",Value(line));
	//CONTENT
	std::getline(t,line);
	signal->set_property_raw("content",Value(line));

	return signal;
}

void writeSignal(const char* filename, Object* signal)
{
	std::ofstream sigfile = std::ofstream(filename);
	std::string outstr = {PACKET_VERSION,'\n'};

	//source
	outstr += signal->get_property_raw("source").to_string();
	outstr.push_back('\n');
	//freq
	Value tmp = signal->get_property_raw("freq");
	if(tmp.t_vType == Value::vType::Integer)
		outstr += tmp.t_value.as_int;
	else if(tmp.t_vType == Value::vType::Double)
	{
		outstr += static_cast<int>(tmp.t_value.as_double);
	}
	else
	{
		outstr += '0';
	}
	outstr.push_back('\n');

	//job
	outstr += signal->get_property_raw("job").to_string();
	outstr.push_back('\n');

	//content
	outstr += signal->get_property_raw("content").to_string();
	//outstr.push_back('\n');

	sigfile.write(outstr.c_str(),outstr.length());
	sigfile.close(); // Pretty sure these streams do this on destruct anyways but, whatever. Explicitness and whatnot.
}

void dumpError(const char* filename, const std::string& what, joaoErrorCode code)
{
	std::ofstream errfile = std::ofstream(filename);
	std::string outstr = "ERROR\n"+ what + std::string("\n") + std::to_string(static_cast<uint8_t>(code));
	errfile.write(outstr.c_str(),std::min(static_cast<size_t>(1 << 10),outstr.length())); // 1024 bytes of room to explain yourself
	errfile.close();
}

int main(int argc, char** argv)
{
	
	//Arg processing
	//Expected args:
	//1. Location of FIFO pipe containing signal packet
	//2. Location of Joao script to be executed
	if(argc != 2)
		exit(1);

	std::string filestr = std::string(argv[1]);

	//Typical execution of a file
	Program parsed;

	std::ifstream scriptfile;

	scriptfile.open(filestr);
	if (!scriptfile.good())
	{
		dumpError(argv[0],"Unable to open file " + filestr + "!\n",TransferError);
		exit(TransferError);
	}

	Scanner scn;
	try
	{
		
		scn.scan(scriptfile);
		scriptfile.close();
	}
	catch(error::scanner s_err)
	{
		//ByondValue& error = packet.get("err");
		//error.set("what", s_err.what());
		dumpError(argv[0],s_err.what(),joaoErrorCode::ScannerError);
		exit(joaoErrorCode::ScannerError);
	}
	Parser pears(scn);

	//Telecomms setup
	ObjectType signal_type("signal");
	Value empty_string = Value(std::string(""));
	signal_type.set_typeproperty_raw("content", empty_string);
	signal_type.set_typeproperty_raw("freq", Value(0));
	signal_type.set_typeproperty_raw("source", empty_string);

	try
	{
		parsed = pears.parse();
	}
	catch(error::parser p_err)
	{
		dumpError(argv[0],p_err.what(),joaoErrorCode::ParserError);
		exit(joaoErrorCode::ParserError);
	}

	Interpreter interpreter(parsed,false);
	
	Object* signal = makeSignal(interpreter,argv[0],signal_type);

	//Lets populate the arguments to /main()
	std::vector<Value> joao_args;
	joao_args.push_back(Value(signal));
	Value jargs = interpreter.makeBaseTable(joao_args,{},nullptr);

	//Execute!
	Value ret;
	try
	{
		ret = interpreter.execute(parsed, jargs);
	}
	catch(const Value& err)
	{
		if(err.t_vType != Value::vType::Object)
		{
			dumpError(argv[0],"Runtime threw with Value not of type /error!",joaoErrorCode::UnknownError);
			exit(joaoErrorCode::UnknownError);
		}
		Object* errobj =err.t_value.as_object_ptr;
		dumpError(argv[0],*(errobj->get_property_raw("what").t_value.as_string_ptr),RuntimeError); //FIXME: We should care about the .code property :(
		exit(RuntimeError);
	}
	catch(std::exception exp)
	{
		dumpError(argv[0],exp.what(),joaoErrorCode::FatalError);
		exit(FatalError);
	}
	catch(...)
	{
		dumpError(argv[0],"Interpreter crashed with unknown error!",joaoErrorCode::UnknownError);
		exit(joaoErrorCode::UnknownError);
	}
	if (ret.t_vType != Value::vType::Object || ret.t_value.as_object_ptr->object_type != "signal") // If the program returned something that isn't a signal object
	{
		dumpError(argv[0],"/main() returned with Value not of type /signal!",joaoErrorCode::FatalError);
		exit(FatalError);
	}

	writeSignal(argv[0],ret.t_value.as_object_ptr);
	return joaoErrorCode::NoError;
}