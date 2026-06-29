#pragma once

// C++
#include <memory>
#include <string_view>

// cosmos
#include <cosmos/dso_export.h>

struct utsname;

namespace cosmos {

/// Access to operating system information strings.
/**
 * This structure provides access to the Linux kernel release, hostname and
 * hardware identifiers as reported by the Linux kernel.
 *
 * The strings obtained are plain C strings without length information.  For
 * this reason string_view objects are returned here. These objects are only
 * valid as long as the Uname instance they were retrieved from is valid.
 **/
class COSMOS_API Uname {
public: // functions

	/// Fetch the system information.
	/**
	 * Since this performs a system an ApiError can be thrown.
	 **/
	explicit Uname();

	~Uname();

	/// The operating system name ("Linux")
	std::string_view sysName() const;

	/// The hostname used for networking
	std::string_view nodeName() const;

	/// The NIS or yellow-page domainname.
	std::string_view domainName() const;

	/// The kernel version number (like "6.1.4").
	std::string_view release() const;

	/// Returns a custom kernel build string (like the build date, version suffix etc.).
	std::string_view version() const;

	/// Returns a hardware identifier (like "x86_64").
	std::string_view machine() const;

protected: // data

	std::unique_ptr<struct utsname> m_buf;
};

} // end ns
