// cosmos
#include <cosmos/string.hxx>

// C++
#include <algorithm>
#include <cctype>
#include <ostream>

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

std::wstring to_lower(const std::wstring_view s) {
	std::wstring ret;
	ret.resize(s.size());
	// put it all to lower case
	std::transform(
		s.begin(), s.end(),
		ret.begin(),
		std::towlower
	);

	return ret;
}

std::wstring to_upper(const std::wstring_view s) {
	std::wstring ret;
	ret.resize(s.size());
	// put it all to lower case
	std::transform(
		s.begin(), s.end(),
		ret.begin(),
		std::towupper
	);

	return ret;
}

void strip(std::string &s) {
	while (!s.empty() && std::isspace(s[0]))
		s.erase(s.begin());

	while (!s.empty() && std::isspace(s.back()))
		s.pop_back();
}

void strip(std::wstring &s) {
	while (!s.empty() && std::iswspace(s[0]))
		s.erase(s.begin());

	while (!s.empty() && std::iswspace(s.back()))
		s.pop_back();
}

template <typename CHAR>
std::vector<std::basic_string<CHAR>> split(
		const std::basic_string_view<CHAR> str,
		const std::basic_string_view<CHAR> sep,
		const SplitFlags flags,
		const size_t max_splits) {

	using String = std::basic_string<CHAR>;
	std::vector<String> parts;

	// index of current start of token
	size_t pos1;
	// index of current end of token
	size_t pos2 = 0;

	while(true) {
		String token;

		pos1 = pos2;

		if (!flags[SplitFlag::KEEP_EMPTY]) {
			while (str.substr(pos1, sep.size()) == sep)
				pos1 += sep.size();
		}

		const auto max_reached = max_splits &&
			parts.size() == max_splits;

		pos2 = max_reached ? str.npos : str.find(sep, pos1);

		auto token_len = pos2 - pos1;

		if (token_len) {
			token = str.substr(pos1, token_len);

			if (flags[SplitFlag::STRIP_PARTS]) {
				strip(token);
			}
		}

		if (!token.empty() || flags[SplitFlag::KEEP_EMPTY]) {
			parts.push_back(token);
		}

		if (pos2 == str.npos)
			break;

		pos2 += sep.size();
	}

	return parts;
}

/* explicit instantiations for outlined template functions */

template COSMOS_API std::vector<std::basic_string<char>> split(
		const std::basic_string_view<char> str,
		const std::basic_string_view<char> sep,
		const SplitFlags flags,
		const size_t);

} // end ns
