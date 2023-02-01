#ifndef COSMOS_STRING_HXX
#define COSMOS_STRING_HXX

// C++
#include <cstring>
#include <ostream>
#include <string>
#include <vector>

namespace cosmos {

//! returns an all lower case version of \c s
std::string COSMOS_API tolower(const std::string &s);
/// returns an all upper case veersion of \c s
std::string COSMOS_API toupper(const std::string &s);

//! strips leading and trailing whitespace from the given string in-place
void COSMOS_API strip(std::string &s);

//! returns a version of the given string with stripped off leading and trailing whitespace
inline std::string stripped(const std::string &s) {
	auto ret(s);
	strip(ret);
	return ret;
}

//! comparison type for std::map and similar with plain char* as keys
struct compare_cstring {
	bool operator()(const char *a, const char *b) const
	{ return std::strcmp(a, b) < 0; }
};

/**
 * \brief
 * 	Returns whether \c prefix is a prefix of \c s
 **/
inline bool isPrefix(const std::string &s, const std::string &prefix) {
	return s.substr(0, prefix.length()) == prefix;
}

} // end ns

#endif // inc. guard
