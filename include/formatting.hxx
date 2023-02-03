#ifndef COSMOS_FORMATTING_HXX
#define COSMOS_FORMATTING_HXX

// stdlib
#include <iomanip>
#include <ostream>
#include <string>
#include <sstream>
#include <type_traits>

// cosmos
#include "cosmos/algs.hxx"
#include "cosmos/compiler.hxx"
#include "cosmos/ostypes.hxx"

/**
 * @file
 *
 * C stdio and C++ iostream related helper types and functions. Also some
 * output operators for cosmos primitive types.
 **/

namespace cosmos {

/// Helper to output a primitive integer as hexadecimal
/**
 * the stream state will be maintained i.e. after the output operation
 * finished the previous/default stream state will be applied
 **/
template <typename NUM>
struct hexnum {
	explicit hexnum(const NUM num, size_t width) : m_num(num), m_width(width) {}

	/// If we should prefix '0x' to the number (default yes)
	const hexnum& showBase(bool yes_no) { m_show_base = yes_no; return *this; }

	size_t getWidth() const { return m_width; }
	bool showBase() const { return m_show_base; }
	auto getNum() const { return m_num; }

	explicit operator std::string() const {
		std::stringstream ss;
		ss << *this;
		return ss.str();
	}
protected: // data

	NUM m_num;
	size_t m_width = 0;
	bool m_show_base = true;
};

/// This is a C++ variant of the libc sprintf() function
COSMOS_API std::string sprintf(const char *fmt, ...) COSMOS_FORMAT_PRINTF(1, 2);

/// This helper makes sure that any integer is turned into a printable integer
/**
 * Attempting to output a `char` related type onto an ostream will print its
 * symbolic value as opposed to its numerical representation. To avoid this
 * effect this helper function can be used to return a representation of num
 * that will be printed as a numerical value when output onto an ostream.
 **/
template <typename T>
auto to_printable_integer(T num) -> decltype(+num) {
	// using the uniary + operator promotes the number to a printable type
	return +num;
}

} // end ns cosmos

template <typename NUM>
std::ostream& operator<<(std::ostream &o, const cosmos::hexnum<NUM> &hn) {
	const auto orig_flags = o.flags();
	const auto orig_fill = o.fill();

	static_assert(std::is_integral_v<NUM>, "template type needs to be an integral integer type");

	// don't handle this with std::showbase, it's behaving badly e.g. the
	// "0x" prefix counts towards the field with, also, the fill character
	// will be prepended to the "0x" prefix resulting in things like
	// "0000x64".
	if (hn.showBase())
		o << "0x";
	o << std::setw(hn.getWidth()) << std::hex << std::setfill('0')
		<< cosmos::to_printable_integer(hn.getNum());

	o.flags(orig_flags);
	o.fill(orig_fill);
	return o;
}

inline std::ostream& operator<<(std::ostream &o, const cosmos::ProcessID &pid) {
	// we could also think about using a consistent annotation of process
	// ids in the output like @1234 or <pid: 1234> something like that.
	o << cosmos::to_integral(pid);
	return o;
}

inline std::ostream& operator<<(std::ostream &o, const cosmos::UserID &uid) {
	// similarly we could use special annotation here
	o << cosmos::to_integral(uid);
	return o;
}

inline std::ostream& operator<<(std::ostream &o, const cosmos::GroupID &gid) {
	// similarly we could use special annotation here
	o << cosmos::to_integral(gid);
	return o;
}

inline std::ostream& operator<<(std::ostream &o, const cosmos::SignalNr &sig) {
	// similarly we could use special annotation here
	o << cosmos::to_integral(sig);
	return o;
}

#endif // inc. guard
