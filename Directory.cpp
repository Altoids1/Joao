#include "Forward.h"
#include "Directory.h"

std::string Directory::DotDot(std::string dir) // Go up one directory from this directory, i.e. "/apple/pear" -> "/apple"
{
	size_t lastslash = dir.find_last_of('/');
	if (lastslash == std::string::npos)
		return "";
	return dir.erase(lastslash, std::string::npos);
}

std::string Directory::lastword(std::string dir)
{
	size_t lastslashpos = dir.find_last_of('/');

	if (lastslashpos == std::string::npos) // If this directory lacks a slash (???)
	{
		return dir; // return itself
	}
	else if (dir[lastslashpos] == dir.back()) // If the slash is at the very end
	{
		return "";
	}

	return dir.substr(lastslashpos + 1, std::string::npos);
}