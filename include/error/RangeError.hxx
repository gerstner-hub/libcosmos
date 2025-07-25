#pragma once

// cosmos
#include <cosmos/error/ApiError.hxx>

namespace cosmos {

/// Specialized exception type for out of range errors reported by the OS.
/**
 * This is a dedicated error type, because it can carry a hint about the
 * supported range returned from a system or libc call.
 **/
class COSMOS_API RangeError :
		public ApiError {
public: // functions

	explicit RangeError(const std::string_view operation,
				const size_t required_length = 0,
				const SourceLocation &src_loc =
					SourceLocation::current()) :
			ApiError{operation, {}, src_loc},
			m_required_length{required_length} {

	}

	auto requiredLength() const {
		return m_required_length;
	}

	bool requiredLengthKnown() const {
		return requiredLength() != 0;
	}

protected: // data

	const size_t m_required_length = 0;
};

} // end ns
