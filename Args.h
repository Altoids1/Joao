#pragma once

#include "Forward.h"

namespace Args
{
	void print_version();
	void print_help();

	bool run_code_block(std::vector<std::string*>&);

	void interactive_mode();
}