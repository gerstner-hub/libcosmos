// Cosmos
#include "cosmos/io/iovector.hxx"

namespace cosmos {

static_assert(sizeof(struct iovec_const) == sizeof(struct iovec),
		"size mismatch between iovec_const vs. struct iovec in system headers");

} // end ns
