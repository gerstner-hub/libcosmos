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

	explicit InternalError(const std::string_view msg,
				const SourceLocation &src_loc =
					SourceLocation::current()) :
			CosmosError{"InternalError", {}, src_loc} {
		m_msg = msg;
	}
};

} // end ns
