#ifndef COSMOS_APIERROR_HXX
#define COSMOS_APIERROR_HXX

// C++
#include <iosfwd>
#include <optional>
#include <string>

// cosmos
#include "cosmos/errors/errno.hxx"
#include "cosmos/ostypes.hxx"
#include "cosmos/errors/CosmosError.hxx"

namespace cosmos {

/// Specialized exception type used for when system APIs fail
/**
 * This exception type will store a well known errno code as a member and
 * format a human readable error message from it.
 **/
class COSMOS_API ApiError :
		public CosmosError {
public: // functions

	/// Stores the current errno code in the exception
	explicit ApiError(const std::optional<std::string_view> prefix = {});

	/// Stores the given errno code in the exception
	explicit ApiError(const Errno err);

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

COSMOS_API std::ostream& operator<<(std::ostream &o, const cosmos::Errno err);

#endif // inc. guard
