#ifndef COSMOS_WOULDBLOCK_HXX
#define COSMOS_WOULDBLOCK_HXX

// cosmos
#include "cosmos/error/ApiError.hxx"

namespace cosmos {

/// Specialized ApiError for handling non-blocking operation
/**
 * When using APIs in non-blocking mode then Errno::AGAIN or
 * Errno::WOULD_BLOCK frequently occur when no data is available. To handle
 * these situations more expressively this specialized exception type is
 * provided that allows to catch this context explicitly without having to
 * check for special Errno values in ApiError.
 **/
class COSMOS_API WouldBlock :
		public ApiError {
public:
	WouldBlock(const std::optional<std::string_view> prefix = {}) :
		ApiError{prefix}
	{}

	COSMOS_ERROR_IMPL;
};

} // end ns

#endif // inc. guard
