// Cosmos
#include "cosmos/errors/ApiError.hxx"
#include "cosmos/time/Clock.hxx"

namespace cosmos {

void ClockBase::nowFromClock(TimeSpec &ts, ClockType clock) const {
	auto res = clock_gettime(static_cast<clockid_t>(clock), &ts);

	if (res != 0) {
		cosmos_throw (ApiError());
	}
}

} // end ns
