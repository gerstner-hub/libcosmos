#ifndef COSMOS_FORMATTING_HXX
#define COSMOS_FORMATTING_HXX

// stdlib
#include <iomanip>
#include <ostream>
#include <string>

// cosmos
#include "cosmos/compiler.hxx"

namespace cosmos {

/// helper to output a primitive integer as hexadecimal
/**
 * the stream state will be maintained i.e. after the output operation
 * finished the previous/default stream state will be applied
 **/
template <typename NUM>
struct hexnum {
	// TODO: BUG: if NUM is a `char` type then this breaks and outputs
	// characters instead of numbers
	explicit hexnum(const NUM &num, size_t width) : m_num(num), m_width(width) {}

	/// if we should prefix '0x' to the number (default yes)
	const hexnum& showBase(bool yes_no) { m_show_base = yes_no; return *this; }

	size_t getWidth() const { return m_width; }
	bool showBase() const { return m_show_base; }
	auto getNum() const { return m_num; }

protected: // data

	NUM m_num;
	size_t m_width = 0;
	bool m_show_base = true;
};

/// this is a C++ variant of the libc sprintf() function
COSMOS_API std::string sprintf(const char *fmt, ...) COSMOS_FORMAT_PRINTF(1, 2);

}

template <typename NUM>
std::ostream& operator<<(std::ostream &o, const cosmos::hexnum<NUM> &hn) {
	const auto orig_flags = o.flags();
	const auto orig_fill = o.fill();

	// don't handle this with std::showbase, it's behaving badly e.g. the
	// "0x" prefix counts towards the field with, also, the fill character
	// will be prepended to the "0x" prefix resulting in things like
	// "0000x64".
	if (hn.showBase())
		o << "0x";
	o << std::setw(hn.getWidth()) << std::hex << std::setfill('0') << hn.getNum();

	o.flags(orig_flags);
	o.fill(orig_fill);
	return o;
}

#endif // inc. guard
