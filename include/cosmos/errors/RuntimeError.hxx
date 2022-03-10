#ifndef COSMOS_RUNTIMEERROR_HXX
#define COSMOS_RUNTIMEERROR_HXX

// cosmos
#include "cosmos/errors/CosmosError.hxx"

namespace cosmos {

/**
 * \brief
 * 	Exception type for generic runtime errors
 * \details
 * 	To be used in case where not an immediate system call failed but other
 * 	logical conditions are violated that make continuing impossible.
 **/
class COSMOS_API RuntimeError :
	public CosmosError
{
public: // functions

	explicit RuntimeError(const std::string_view &msg) : CosmosError("RuntimeError") {
		m_msg = msg;
	}

	COSMOS_ERROR_IMPL;
};

} // end ns

#endif // inc. guard
