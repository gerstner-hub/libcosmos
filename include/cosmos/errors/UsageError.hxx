#ifndef COSMOS_USAGEERROR_HXX
#define COSMOS_USAGEERROR_HXX

// cosmos
#include "cosmos/errors/CosmosError.hxx"

namespace cosmos {

/**
 * \brief
 * 	Exception type for logical usage errors within the application
 **/
class COSMOS_API UsageError :
	public CosmosError
{
public: // functions

	explicit UsageError(const char *msg) :
		CosmosError("UsageError"),
		m_error_msg(msg)
	{}

	explicit UsageError(const std::string &msg) :
		UsageError(msg.c_str())
	{}

	[[ noreturn ]] void raise() override { throw *this; }

protected: // functions

	void generateMsg() const override { m_msg += m_error_msg; }

protected: // data

	std::string m_error_msg;
};

} // end ns

#endif // inc. guard
