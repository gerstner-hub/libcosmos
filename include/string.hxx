#ifndef COSMOS_STRING_HXX
#define COSMOS_STRING_HXX

// C++
#include <cctype>
#include <cstring>
#include <string>
#include <string_view>
#include <vector>

// cosmos
#include "cosmos/dso_export.h"
#include "cosmos/BitMask.hxx"

namespace cosmos {

/**
 * @file
 *
 * This header contains helper functions and types for dealing with
 * std::string objects and general string processing topics.
 **/

/// Comparison type for std::map and similar containers with plain char* as keys.
struct CompareCString {
	bool operator()(const char *a, const char *b) const {
		return std::strcmp(a, b) < 0;
	}
};

/// Returns an all lower case version of \c s.
std::string COSMOS_API to_lower(const std::string_view s);
/// Returns an all upper case veersion of \c s.
std::string COSMOS_API to_upper(const std::string_view s);

/// Strips leading and trailing whitespace from the given string in-place.
void COSMOS_API strip(std::string &s);

/// Returns a version of the given string with stripped off leading and trailing whitespace.
inline std::string stripped(const std::string_view s) {
	std::string ret{s};
	strip(ret);
	return ret;
}

/// Returns whether \c prefix is a prefix of \c s.
inline bool is_prefix(const std::string_view s, const std::string_view prefix) {
	// TODO: in C++20 string_view has a starts_with() member
	return s.substr(0, prefix.length()) == prefix;
}

/// Returns whether /\c suffix is a suffix oc \c s.
inline bool is_suffix(const std::string_view s, const std::string_view suffix) {
	if (s.size() < suffix.size())
		return false;
	// TODO: in C++20 string_view has an ends_with() member
	return s.substr(s.size() - suffix.size()) == suffix;
}

/// Simple wrapper that creates a string_view from \c s and supporting also nullptr.
/**
 * The std::string_view constructor does not allow a nullptr argument. This
 * wrapper makes this limitation transparent by returning an empty string_view
 * for nullptr C strings.
 **/
inline std::string_view to_string_view(const char *s) {
	return s ? std::string_view{s} : std::string_view{};
}

/// Simple wrapper around std::isprint that implicitly casts to unsigned char.
/**
 * std::isprint is undefined if the passed value is not an unsigned 8 bit type
 * or EOF. Thus cast different character types to unsigned char in this
 * wrapper to avoid this trap.
 **/
template <typename CH>
inline bool printable(CH ch) {
	static_assert(sizeof(ch) == 1, "only character types of one byte size allowed");
	return std::isprint(static_cast<unsigned char>(ch));
}

/// Bitmask for algorithm settings of the split() function.
enum class SplitOpts {
	/// Strip each extracted part using strip().
	STRIP_PARTS = 0x1,
	/// When multiple subsequent separators occur, keep empty parts in the result.
	KEEP_EMPTY  = 0x2
};

using SplitFlags = BitMask<SplitOpts>;

/// Split a string at separator boundaries.
/**
 * The input string \c str will be split up at every occurence of the \c sep
 * substring. The resulting parts are returned as a vector. By default
 * subsequent occurences of \c sep are ignored. If SplitOpts::KEEP_EMPTY is
 * set in \c flags then empty elements will be returned for these occurences
 * instead.
 **/
template <typename CHAR>
COSMOS_API std::vector<std::basic_string<CHAR>> split(
		const std::basic_string_view<CHAR> str,
		const std::basic_string_view<CHAR> sep,
		const SplitFlags flags = SplitFlags{});

// overload of split avoid type deduction problems
inline std::vector<std::string> split(
		const std::string_view str,
		const std::string_view sep,
		const SplitFlags flags = SplitFlags{}) {
	return split<char>(str, sep, flags);
}

} // end ns

#endif // inc. guard
