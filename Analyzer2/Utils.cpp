#include "Utils.h"

std::string regex_escape(const std::string& s)
{
	const std::regex metacharacters(R"([-[\]{}()*+?.,\^$|#\s])");
	return std::regex_replace(s, metacharacters, "\\$&");
}