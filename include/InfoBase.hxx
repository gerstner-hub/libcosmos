#pragma once

// C++
#include <functional>
#include <string_view>
#include <vector>

namespace cosmos {

/// Base class for PasswdInfo and GroupInfo.
template <typename DB_STRUCT>
class InfoBase {
public: // functions

	/// Returns whether data is present in the object.
	/**
	 * If no corresponding entry was found during construction time then
	 * this returns `false`.
	 **/
	bool valid() const { return m_valid; }

	/// Zeroes out all data.
	void reset();

	/// Grants access to the raw underlying data structure.
	const DB_STRUCT* raw() const { return &m_info; }
	DB_STRUCT* raw() { return &m_info; }

protected: // functions

	/// Helper to drive the common getter function logic for getpw* and getgr*.
	bool getInfo(std::function<int(DB_STRUCT**)> get_func, const char *errlabel);

protected: // data

	bool m_valid = false;
	/// struct passwd or struct group.
	DB_STRUCT m_info;
	/// Extra space for storing the dynamic strings in the m_info struct.
	std::vector<char> m_buf;
};

} // end ns
