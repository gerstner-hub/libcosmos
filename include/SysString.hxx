#pragma once

// C++
#include <cstring>
#include <iosfwd>
#include <string>
#include <string_view>

// Cosmos
#include "cosmos/dso_export.h"

namespace cosmos {

/// Wrapper type around a C-style string for use with system APIs.
/**
 * This type is used in place of a plain `const char*` to pass string data to
 * system APIs and vice versa.
 *
 * This type can be constructed from a plain `const char*` or from a
 * `std::string`. It cannot be constructed from a `std::string_view()`, since
 * this would need to create a copy. `std::string_view` needs not to be null
 * terminated, but system APIs require null terminated strings. To pass a
 * `std::string_view` you need to explicitly convert it into a `std::string`
 * instead. This is similar to the behaviour of STL APIs where an implicit
 * conversion from `std::string_view` to `std::string` is not available.
 *
 * This is only a lightweight container that is mostly intended for passing
 * strings back and forth to lower level APIs. There is no ownership tracking
 * here, the object only carries a flat copy of whatever string was passed
 * into it. So take care not to use an instance of this type after the backing
 * object has lost validity.
 **/
struct SysString {

	SysString() = default;

	SysString(const std::string &str) :
			m_ptr{str.c_str()} {}

	SysString(const char *str) :
			m_ptr{str} {}

	const char* raw() const {
		return m_ptr ? m_ptr : "";
	}

	size_t length() const {
		return view().length();
	}

	bool empty() const {
		return !m_ptr || m_ptr[0] == '\0';
	}

	std::string str() const {
		return m_ptr ? std::string{m_ptr} : std::string{};
	}

	std::string_view view() const {
		return m_ptr ? std::string_view{m_ptr} : std::string_view{};
	}

	operator std::string() const {
		return str();
	}

	operator std::string_view() const {
		return view();
	}

	SysString& operator=(const char *str) {
		m_ptr = str;
		return *this;
	}

	SysString& operator=(const std::string &str) {
		m_ptr = str.c_str();
		return *this;
	}

	bool operator==(const SysString &other) const {
		if (m_ptr == other.m_ptr)
			return true;

		if (!m_ptr || !other.m_ptr)
			return false;

		return std::strcmp(m_ptr, other.m_ptr) == 0;
	}

	bool operator!=(const SysString &other) const {
		return !(*this == other);
	}

protected: // data

	const char *m_ptr = nullptr;
};

} // end ns

COSMOS_API std::ostream& operator<<(std::ostream &o, const cosmos::SysString str);

inline bool operator==(const std::string &s, const cosmos::SysString str) {
	return s == str.view();
}

inline bool operator!=(const std::string &s, const cosmos::SysString str) {
	return !(s == str);
}

inline bool operator==(const std::string_view &s, const cosmos::SysString str) {
	return s == str.view();
}

inline bool operator!=(const std::string_view &s, const cosmos::SysString str) {
	return !(s == str);
}
