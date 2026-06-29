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

	/// Create the object and optionally fetch the system information.
	/**
	 * If `fetch_data` is set then update() is called right away.
	 * Otherwise empty strings will be stored in the object.
	 **/
	explicit Uname(const bool fetch_data = true);

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

	/// Fetch current information from the kernel into the struct.
	/**
	 * This operation can thrown an ApiError (only in case of memory
	 * corruption).
	 **/
	void update();

protected: // data

	std::unique_ptr<struct utsname> m_buf;
};

} // end ns
