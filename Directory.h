#pragma once

/*
A namespace of helper functions for the manipulation of strings that indicate a directory.
*/

namespace Directory
{
	std::string DotDot(std::string dir) // Go up one directory from this directory
	{
		size_t lastslash = dir.find_last_of('/');
		if (lastslash == std::string::npos)
			return "";
		return dir.erase(lastslash, std::string::npos);
	}
}