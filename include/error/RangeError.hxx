#pragma once

// cosmos
#include "cosmos/error/ApiError.hxx"

namespace cosmos {

class COSMOS_API RangeError :
		public ApiError {
public: // functions

	explicit RangeError(const std::string_view operation, const size_t required_length = 0) :
			ApiError{operation},
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
