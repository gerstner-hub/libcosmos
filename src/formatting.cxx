// libc
#include <stdarg.h>

// C++
#include <cstdint>
#include <climits>
#include <string>

// cosmos
#include <cosmos/formatting.hxx>

namespace cosmos {

namespace {

/* silence clang warning about non-literal fmt */ COSMOS_FORMAT_PRINTF(1, 0)
std::string sprintf_v(const char *fmt, va_list orig_args) {
	std::string ret;
	// let's use some arbitrary start size that should suffice for average uses
	// and it will be increased if its not enough
	ret.resize(128);
	int written = 0;
	va_list varargs;

	while (true) {
		// copy the list of arguments in any case, since we
		// potentially need to evaluate it multiple times
		va_copy(varargs, orig_args);

		written = vsnprintf(&ret[0], ret.size() + 1, fmt, varargs);

		if (written < 0) {
			// some fatal error, simply return an empty string
			// then
			ret.clear();
			break;
		} else if (static_cast<size_t>(written) <= ret.size()) {
			// the return value is excluding the null terminator
			// so it's just rigth for resize()
			ret.resize(written);
			break;
		}

		// re-try with the correct string size
		ret.resize(written);

		va_end(varargs);
	}

	va_end(varargs);

	return ret;
}

} // end anon ns

std::string sprintf(const char *fmt, ...) {
	va_list varargs;
	va_start(varargs, fmt);
	const auto ret = sprintf_v(fmt, varargs);
	va_end(varargs);
	return ret;
}

} // end ns

template <typename NUM>
std::ostream& operator<<(std::ostream& o, const cosmos::FormattedNumber<NUM> &fmtnum) {
	const auto orig_flags = o.flags();
	const auto orig_fill = o.fill();

	static_assert(std::is_integral_v<NUM>, "template type needs to be an integral integer type");

	// don't handle this with std::showbase, it's behaving badly e.g. the
	// "0x" prefix counts towards the field with, also, the fill character
	// will be prepended to the "0x" prefix resulting in things like
	// "0000x64".
	if (fmtnum.showBase())
		o << fmtnum.basePrefix();
	o << std::setw(fmtnum.width());
	fmtnum.baseFN()(o);
	o << std::setfill('0') << cosmos::to_printable_integer(fmtnum.num());

	o.flags(orig_flags);
	o.fill(orig_fill);
	return o;
}

/* explicit instantiations of the templated operator<< */

template COSMOS_API std::ostream& operator<<(std::ostream&, const cosmos::FormattedNumber<unsigned int>&);
template COSMOS_API std::ostream& operator<<(std::ostream&, const cosmos::FormattedNumber<unsigned short>&);
template COSMOS_API std::ostream& operator<<(std::ostream&, const cosmos::FormattedNumber<int>&);
template COSMOS_API std::ostream& operator<<(std::ostream&, const cosmos::FormattedNumber<char>&);

// on 32-bit archs these can be the same, causing a duplicate instantiation
#if SIZE_MAX != UINT_MAX
template COSMOS_API std::ostream& operator<<(std::ostream&, const cosmos::FormattedNumber<size_t>&);
// similarly this ull definition is then missing on 32-bit archs, provide it
#else
template COSMOS_API std::ostream& operator<<(std::ostream&, const cosmos::FormattedNumber<unsigned long long>&);
template COSMOS_API std::ostream& operator<<(std::ostream&, const cosmos::FormattedNumber<unsigned long>&);
#endif
