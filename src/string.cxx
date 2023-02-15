// cosmos
#include "cosmos/string.hxx"

// C++
#include <algorithm>
#include <cctype>

namespace cosmos {

std::string to_lower(const std::string_view s) {
	std::string ret;
	ret.resize(s.size());
	// put it all to lower case
	std::transform(
		s.begin(), s.end(),
		ret.begin(),
		::tolower
	);

	return ret;
}

std::string to_upper(const std::string_view s) {
	std::string ret;
	ret.resize(s.size());
	// put it all to lower case
	std::transform(
		s.begin(), s.end(),
		ret.begin(),
		::toupper
	);

	return ret;
}

void strip(std::string &s) {
	while (!s.empty() && std::isspace(s[0]))
		s.erase(s.begin());

	while (!s.empty() && std::isspace(s[s.length()-1]))
		s.pop_back();
}

} // end ns
