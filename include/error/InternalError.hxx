#pragma once

// cosmos
#include <cosmos/error/CosmosError.hxx>

namespace cosmos {

/// Exception type for grave internal errors.
/**
 * To be used in case e.g. elemental preconditions that are considered a given
 * are not fulfilled.
 **/
class COSMOS_API InternalError :
		public CosmosError {
public: // functions

	explicit InternalError(const std::string_view msg) :
			CosmosError{"InternalError"} {
		m_msg = msg;
	}

	COSMOS_ERROR_IMPL;
};

} // end ns
