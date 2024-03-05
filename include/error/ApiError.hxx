#pragma once

// C++
#include <string>

// cosmos
#include <cosmos/error/CosmosError.hxx>
#include <cosmos/error/errno.hxx>

namespace cosmos {

/// Specialized exception type used when system APIs fail.
/**
 * This exception type will store a well known errno code as a member and
 * format a human readable error message from it.
 **/
class COSMOS_API ApiError :
		public CosmosError {
public: // functions

	/// Stores the current errno code in the exception
	explicit ApiError(const std::string_view prefix);

	/// Stores the given errno code in the exception
	ApiError(const std::string_view prefix, const Errno err);

	/// Returns the plain operating system error message
	std::string msg() const { return msg(m_errno); }

	/// Returns a human readable error message for the given errno code
	static std::string msg(const Errno err);

	/// Returns the plain errno stored in the exception
	auto errnum() const { return m_errno; }

	COSMOS_ERROR_IMPL;

protected: // functions

	void generateMsg() const override;

protected: // data

	Errno m_errno = Errno::NO_ERROR;
};

} // end ns
