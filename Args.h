#pragma once

#include "Forward.h"

namespace Args
{
	enum class Flags
	{
		Interactive,
		Version,
		Help,
		Main,
		Executetime,
		InitializeDaemon
	};

	//Returns the string which is the actual filename.
	std::string read_args(std::vector<Flags>& v, int argc, char** argv, int& file_start);

	void print_version();
	void print_help();
	void interactive_mode();
}