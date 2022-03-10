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

	explicit UsageError(const std::string_view &msg) : CosmosError("UsageError") {
		m_msg = msg;
	}

	COSMOS_ERROR_IMPL;
};

} // end ns

#endif // inc. guard
