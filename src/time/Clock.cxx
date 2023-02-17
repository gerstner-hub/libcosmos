// C++
#include <type_traits>

// Cosmos
#include "cosmos/algs.hxx"
#include "cosmos/dso_export.h"
#include "cosmos/error/ApiError.hxx"
#include "cosmos/private/cosmos.hxx"
#include "cosmos/time/Clock.hxx"

namespace cosmos {

// this is an important property since we want to take advantage of wrapping
// existing struct timespec instances e.g. in FileStatus::getModeTime()
static_assert(sizeof(RealTime) == sizeof(struct timespec));
static_assert(sizeof(MonotonicTime) == sizeof(struct timespec));

// this is sadly not true, because the type has custom constructors
#if 0
static_assert(std::is_trivial<RealTime>::value == true);
#endif

template <ClockType CLOCK>
void Clock<CLOCK>::now(TimeSpec<CLOCK> &ts) const {
	auto res = clock_gettime(to_integral(CLOCK), &ts);

	if (res != 0) {
		cosmos_throw (ApiError());
	}
}

template <ClockType CLOCK>
TimeSpec<CLOCK> Clock<CLOCK>::resolution() const {
	TimeSpec<CLOCK> ret;
	auto res = clock_getres(to_integral(CLOCK), &ret);

	if (res != 0) {
		cosmos_throw (ApiError());
	}

	return ret;
}

template <ClockType CLOCK>
void Clock<CLOCK>::setTime(const TimeSpec<CLOCK> t) {
	auto res = clock_settime(to_integral(CLOCK), &t);

	if (res != 0) {
		cosmos_throw (ApiError());
	}
}

template <ClockType CLOCK>
void Clock<CLOCK>::sleep(const TimeSpec<CLOCK> until) const {
	while (true) {
		auto res = clock_nanosleep(
				to_integral(CLOCK),
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

/* explicit instantiations of the necessary clock variants */
template class COSMOS_API Clock<ClockType::ATOMIC_REALTIME>;
template class COSMOS_API Clock<ClockType::BOOTTIME>;
template class COSMOS_API Clock<ClockType::MONOTONIC>;
template class COSMOS_API Clock<ClockType::MONOTONIC_COARSE>;
template class COSMOS_API Clock<ClockType::MONOTONIC_RAW>;
template class COSMOS_API Clock<ClockType::PROCESS_CPUTIME>;
template class COSMOS_API Clock<ClockType::REALTIME>;
template class COSMOS_API Clock<ClockType::REALTIME_COARSE>;
template class COSMOS_API Clock<ClockType::THREAD_CPUTIME>;

} // end ns
