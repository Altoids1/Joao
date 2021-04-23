#pragma once

/*
A namespace of helper functions for the manipulation of strings that indicate a directory.
*/
namespace Directory
{
	// Go up one directory from this directory
	std::string DotDot(std::string);

	// Get the last word token within this dir, returning emptystring if there is none and dir if there's no slash
	std::string lastword(std::string);
}