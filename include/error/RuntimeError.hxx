#pragma once

// cosmos
#include <cosmos/error/CosmosError.hxx>

namespace cosmos {

/// Exception type for generic runtime errors.
/**
 * To be used in cases when not an immediate system call failed but other
 * logical conditions are violated that make continuing impossible.
 **/
class COSMOS_API RuntimeError :
		public CosmosError {
public: // functions

	explicit RuntimeError(const std::string_view msg) :
			CosmosError{"RuntimeError"} {
		m_msg = msg;
	}

	COSMOS_ERROR_IMPL;
};

} // end ns
