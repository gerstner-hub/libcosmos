#ifndef COSMOS_INTERNALERROR_HXX
#define COSMOS_INTERNALERROR_HXX

// cosmos
#include "cosmos/errors/CosmosError.hxx"

namespace cosmos {

/**
 * \brief
 * 	Exception type for grave internal errors
 * \details
 * 	To be used in case e.g. elemental preconditions that are considered a
 * 	given are not fulfilled.
 **/
class COSMOS_API InternalError :
	public CosmosError
{
public: // functions

	explicit InternalError(const char *msg) : CosmosError("InternalError") {
		m_msg = msg;
	}

	explicit InternalError(const std::string &msg) : InternalError(msg.c_str()) {}

	COSMOS_ERROR_IMPL;
};

} // end ns

#endif // inc. guard
