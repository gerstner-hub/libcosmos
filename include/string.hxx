#ifndef COSMOS_STRING_HXX
#define COSMOS_STRING_HXX

// C++
#include <cstring>
#include <string>
#include <string_view>

namespace cosmos {

/**
 * @file
 *
 * This header contains helper functions for dealing with std::string objects.
 **/

/// Returns an all lower case version of \c s
std::string COSMOS_API toLower(const std::string_view s);
/// Returns an all upper case veersion of \c s
std::string COSMOS_API toUpper(const std::string_view s);

/// Strips leading and trailing whitespace from the given string in-place
void COSMOS_API strip(std::string &s);

/// Returns a version of the given string with stripped off leading and trailing whitespace
inline std::string stripped(const std::string_view s) {
	std::string ret{s};
	strip(ret);
	return ret;
}

/// Comparison type for std::map and similar containers with plain char* as keys
struct compare_cstring {
	bool operator()(const char *a, const char *b) const {
		return std::strcmp(a, b) < 0;
	}
};

/// Returns whether \c prefix is a prefix of \c s
inline bool isPrefix(const std::string_view s, const std::string_view prefix) {
	return s.substr(0, prefix.length()) == prefix;
}

/// Simple wrapper that creates a string_view from \c s and supporting also nullptr
/**
 * The std::string_view constructor does not allow a nullptr argument. This
 * wrapper makes this limitation transparent by returning an empty string_view
 * for nullptr C strings.
 **/
inline std::string_view to_string_view(const char *s) {
	return s ? std::string_view{s} : std::string_view{};
}

} // end ns

#endif // inc. guard
