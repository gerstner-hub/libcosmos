#pragma once

// Linux
#include <netdb.h>

// C++
#include <iosfwd>
#include <string_view>

// cosmos
#include <cosmos/error/CosmosError.hxx>
#include <cosmos/error/errno.hxx>

// see errno.hxx for the reason for this
#ifdef NO_DATA
#	undef NO_DATA
#endif

namespace cosmos {

/// Specialized error type for AddressInfoList resolve errors.
/**
 * DNS name resolution in Linux APIs uses a separate error reporting
 * mechanism. This error type covers this mechanism.
 **/
class COSMOS_API ResolveError :
		public CosmosError {
public: // types

	/// Possible resolve error codes that can be stored in ResolveError.
	enum class Code : int {
		/// The specified network host does not have any network addresses in the requested family.
		ADDR_FAMILY = EAI_ADDRFAMILY,
		/// The name server returned a temporary failure indication.
		AGAIN       = EAI_AGAIN,
		/// Bad AddressHints::Flags encountered.
		BAD_FLAGS   = EAI_BADFLAGS,
		/// A permanent failure has been indicated by the nameserver.
		FAIL        = EAI_FAIL,
		/// The requested address family is not supported
		FAMILY      = EAI_FAMILY,
		/// Out of memory.
		MEMORY      = EAI_MEMORY,
		/// The requested network host exists but has no network address defined.
		NO_DATA     = EAI_NODATA,
		/// The node or service is not known; or Flags::NUMERIC_SERVICE was specified and service was not a number.
		NO_NAME     = EAI_NONAME,
		/// The requested service is not available for the requested SocketType.
		SERVICE     = EAI_SERVICE,
		/// The requested SocketType is not supported.
		SOCKTYPE    = EAI_SOCKTYPE,
		/// Other system error, check Errno from systemError().
		SYSTEM      = EAI_SYSTEM,
		/// The buffer pointed to by host or serv was too small (only used in IPAddress::getNameInfo()).
		OVERFLOW    = EAI_OVERFLOW,
	};

public: // functions

	/// Create a ResolveError for the given error code.
	/**
	 * If code is Code::SYSTEM then the current Errno will also be stored
	 * in the exception.
	 **/
	explicit ResolveError(const Code code);

	/// Returns the plain resolve error code stored in the exception.
	Code code() const {
		return m_eai_code;
	}

	/// Returns the "other system error" if code() is Code::SYSTEM.
	/**
	 * If there is no system error then Errno::NO_ERROR is returned.
	 **/
	Errno systemError() const {
		return m_system_errno;
	}

	/// Returns the plain resolver error message.
	std::string_view msg() const { return msg(m_eai_code); }

	static std::string_view msg(const Code code);

	COSMOS_ERROR_IMPL;

protected: // functions

	void generateMsg() const override;

protected: // data

	/// The plain resolve error code.
	Code m_eai_code;
	/// If m_eai_code == Code::EAI_SYSTEM this contains the system error.
	Errno m_system_errno;
};

} // end ns

COSMOS_API std::ostream& operator<<(std::ostream &o, const cosmos::ResolveError::Code code);
