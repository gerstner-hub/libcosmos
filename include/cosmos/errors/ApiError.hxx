#ifndef COSMOS_APIERROR_HXX
#define COSMOS_APIERROR_HXX

// cosmos
#include "cosmos/errors/CosmosError.hxx"

namespace cosmos {

/**
 * \brief
 * 	Specialized exception type used for when system APIs fail
 * \details
 * 	This exception type will store a well known errno code as a member and
 * 	format a human readable error message from it.
 **/
class ApiError :
	public CosmosError
{
public: // functions

	//! stores the current errno code in the exception
	explicit ApiError(const char *prefix = nullptr);

	//! stores the given errno code in the exception
	explicit ApiError(const int p_errno);

	[[ noreturn ]] void raise() override { throw *this; }

	std::string msg() const { return msg(m_errno); }

	/**
	 * \brief
	 * 	Returns a human readable error message for the given errno
	 * 	code
	 **/
	static std::string msg(const int no);

protected: // functions

	void generateMsg() const override;

protected: // data

	const char *m_prefix = nullptr;
	int m_errno = 0;
};

} // end ns

#endif // inc. guard
