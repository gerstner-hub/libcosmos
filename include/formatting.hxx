#ifndef COSMOS_FORMATTING_HXX
#define COSMOS_FORMATTING_HXX

// C++
#include <iomanip>
#include <functional>
#include <ostream>
#include <string>
#include <string_view>
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

/// base class for hexnum and octnum format output helpers
template <typename NUM>
struct fmtnum_base {
protected: // types

	/// this function is supposed to apply the desired number base to the stream
	using SetBaseFN = std::function<void(std::ostream&)>;

protected: // functions

	fmtnum_base(const NUM num, size_t width, SetBaseFN fn, std::string_view base_prefix) :
		m_num{num}, m_width{width}, m_setbase_fn{fn}, m_base_prefix{base_prefix} {}

public: // functions

	/// If we should show a prefix identifier the number's base (e.g. 0x for hex, default: yes)
	const fmtnum_base& showBase(bool yes_no) { m_show_base = yes_no; return *this; }

	size_t getWidth() const { return m_width; }
	bool showBase() const { return m_show_base; }
	auto getNum() const { return m_num; }
	const std::string_view getBasePrefix() const { return m_base_prefix; }
	SetBaseFN getSetBaseFN() const { return m_setbase_fn; }

	explicit operator std::string() const {
		std::stringstream ss;
		ss << *this;
		return ss.str();
	}

protected: // data

	NUM m_num;
	size_t m_width = 0;
	SetBaseFN m_setbase_fn;
	std::string_view m_base_prefix;
	bool m_show_base = true;
};

/// Helper to output a primitive integer as hexadecimal in the style of 0x1234
/**
 * the stream state will be maintained i.e. after the output operation is
 * finished the previous/default stream state will be applied
 *
 * Leading zeroes will be added to reach the desired field width.
 *
 * The given field width will only count towards the actual digits the number
 * consists of. A possible base prefix is not counted towards the field width.
 * I.e. if showBase() is set to \c true then hexnum(0x10, 4) will be printed
 * as: "0x0010".
 **/
template <typename NUM>
struct hexnum :
		public fmtnum_base<NUM> {
	hexnum(const NUM num, size_t width) :
		fmtnum_base<NUM>{num, width, [](std::ostream &o){ o << std::hex; }, "0x"}
	{}
};

/// Helper to output a primitive integer as octal in the style of 0o123
/**
 * \see hexnum
 **/
template <typename NUM>
struct octnum :
		public fmtnum_base<NUM> {
	octnum(const NUM num, size_t width) :
		fmtnum_base<NUM>{num, width, [](std::ostream &o){ o << std::oct; }, "0o"}
	{}
};

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

/// This is a C++ variant of the libc sprintf() function
COSMOS_API std::string sprintf(const char *fmt, ...) COSMOS_FORMAT_PRINTF(1, 2);

} // end ns cosmos

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

inline std::ostream& operator<<(std::ostream &o, const cosmos::FileNum &fd) {
	// similarly we could use special annotation here
	o << cosmos::to_integral(fd);
	return o;
}

// this is implemented outlined with explicit template instantiations for the
// currently necessary primitive types
template <typename NUM>
std::ostream& operator<<(std::ostream& o, const cosmos::fmtnum_base<NUM> &fmtnum);

#endif // inc. guard
