#pragma once

// cosmos
#include <cosmos/error/CosmosError.hxx>

namespace cosmos {

/// Exception type for situations when parsing input data fails.
/**
 * This is an error type used for when parsing OS-related data structures
 * fails e.g. in ancillary socket messages or when parsing files in /proc etc.
 **/
class COSMOS_API ParseError :
		public CosmosError {
public: // functions

	explicit ParseError(const std::string_view msg, const SourceLocation &src_loc =
				SourceLocation::current()) :
			CosmosError{"ParseError", {}, src_loc} {
		m_msg = msg;
	}
};

} // end ns
