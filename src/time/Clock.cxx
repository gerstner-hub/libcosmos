// Cosmos
#include "cosmos/algs.hxx"
#include "cosmos/error/ApiError.hxx"
#include "cosmos/private/cosmos.hxx"
#include "cosmos/time/Clock.hxx"

namespace cosmos {

void ClockBase::nowFromClock(TimeSpec &ts, ClockType clock) const {
	auto res = clock_gettime(to_integral(clock), &ts);

	if (res != 0) {
		cosmos_throw (ApiError());
	}
}

void ClockBase::sleepOnClock(const TimeSpec until, ClockType clock) const {
	while (true) {
		auto res = clock_nanosleep(
				to_integral(clock),
				TIMER_ABSTIME,
				&until,
				nullptr
		);

		const auto err = Errno{res};

		switch(err) {
			default: break;
			case Errno::NO_ERROR: return;
			case Errno::INTERRUPTED: {
				if (auto_restart_syscalls) {
					continue;
				}
				break;
			}
		}

		cosmos_throw (ApiError(err));
	}
}

} // end ns
