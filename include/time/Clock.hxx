#pragma once

// cosmos
#include "cosmos/dso_export.h"
#include "cosmos/time/types.hxx"
#include "cosmos/types.hxx"

namespace cosmos {

/// C++ wrapper around the POSIX clocks and related functions.
/**
 * This is a template to make different clock types incompatible with each
 * other.
 **/
template <ClockType CLOCK>
class Clock {
public: // functions

	/// Retrieve the current value of the clock.
	void now(TimeSpec<CLOCK> &ts) const;

	/// Returns the current value of the clock by value.
	TimeSpec<CLOCK> now() const {
		TimeSpec<CLOCK> ret;
		now(ret);
		return ret;
	}

	/// Returns the resolution/precision of the represented clock.
	/**
	 * The returned TimeSpec represents the smallest time unit that can be
	 * processed / detected by the clock. When calling setTime() then the
	 * used time value is truncated to a multiple of the resolution value
	 * returned from this function.
	 **/
	TimeSpec<CLOCK> resolution() const;

	/// Changes the current time value of the represented clock.
	/**
	 * Elevated permissions are necessary to change most clocks e.g.
	 * CAP_SYS_TIME to change the RealTimeClock. Not all clocks can be
	 * set. If this is the case then an ApiError with Errno::INVALID_ARG
	 * is thrown.
	 **/
	void setTime(const TimeSpec<CLOCK> t);

	/// Suspend execution of the calling thread until the clock reaches the given time.
	/**
	 * The given \c until value is an absolute time in the future until
	 * which the execution of the calling thread is to be suspended. If
	 * \c until is not in the future then this call returns immediately
	 * without entering a suspend state.
	 *
	 * Depending of the libcosmos "automatic restart on interrupt"
	 * setting, this call can be interrupted by signals which will cause
	 * an ApiError with Errno::INTERRUPTED to be thrown.
	 **/
	void sleep(const TimeSpec<CLOCK> until) const;

	static ClockType raw() { return CLOCK; }
};

// the following clocks are explicitly instantiated in the compilation unit
using AtomicRealTimeClock  = Clock<ClockType::ATOMIC_REALTIME>;
using BootTimeClock        = Clock<ClockType::BOOTTIME>;
using CoarseMonotonicClock = Clock<ClockType::MONOTONIC_COARSE>;
using CoarseRealTimeClock  = Clock<ClockType::REALTIME_COARSE>;
using MonotonicClock       = Clock<ClockType::MONOTONIC>;
using ProcessTimeClock     = Clock<ClockType::PROCESS_CPUTIME>;
using RawMonotonicClock    = Clock<ClockType::MONOTONIC_RAW>;
using RealTimeClock        = Clock<ClockType::REALTIME>;
using ThreadTimeClock      = Clock<ClockType::THREAD_CPUTIME>;

extern template class COSMOS_API Clock<ClockType::ATOMIC_REALTIME>;
extern template class COSMOS_API Clock<ClockType::BOOTTIME>;
extern template class COSMOS_API Clock<ClockType::MONOTONIC>;
extern template class COSMOS_API Clock<ClockType::MONOTONIC_COARSE>;
extern template class COSMOS_API Clock<ClockType::MONOTONIC_RAW>;
extern template class COSMOS_API Clock<ClockType::PROCESS_CPUTIME>;
extern template class COSMOS_API Clock<ClockType::REALTIME>;
extern template class COSMOS_API Clock<ClockType::REALTIME_COARSE>;
extern template class COSMOS_API Clock<ClockType::THREAD_CPUTIME>;

} // end ns
