#pragma once

// cosmos
#include <cosmos/error/CosmosError.hxx>

namespace cosmos {

/// Exception type for logical usage errors within the application.
/**
 * This error should be used when the caller of a function has violated
 * logical preconditions and continuing is impossible.
 **/
class COSMOS_API UsageError :
		public CosmosError {
public: // functions

	explicit UsageError(const std::string_view msg) :
			CosmosError{"UsageError"} {
		m_msg = msg;
	}

	COSMOS_ERROR_IMPL;
};

} // end ns
