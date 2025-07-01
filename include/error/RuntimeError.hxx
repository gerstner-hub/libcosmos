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

	explicit RuntimeError(const std::string_view msg,
				const SourceLocation &src_loc =
					SourceLocation::current()) :
			CosmosError{"RuntimeError", {}, src_loc} {
		m_msg = msg;
	}
};

} // end ns
