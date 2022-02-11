#ifndef COSMOS_STRING_HXX
#define COSMOS_STRING_HXX

// C++
#include <cstring>
#include <ostream>
#include <string>
#include <vector>

namespace cosmos {

//! returns an all lower case version of \c s
std::string tolower(const std::string &s);

//! strips leading and trailing whitespace from the given string in-place
void strip(std::string &s);

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

// simple output operator for std::vector, requires T to have an available
// operator<<
template <typename T>
inline std::ostream& operator<<(std::ostream &o, const std::vector<T> &v) {

	bool first = true;

	for (const auto &e: v)  {
		if (first)
			first = false;
		else
			o << " ";

		o << e;
	}

	return o;
}

#endif // inc. guard
