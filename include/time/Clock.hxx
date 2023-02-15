#ifndef COSMOS_CLOCK_HXX
#define COSMOS_CLOCK_HXX

// Linux
#include <time.h>

// cosmos
#include "cosmos/time/TimeSpec.hxx"
#include "cosmos/types.hxx"

namespace cosmos {

/// Available clock types for time operations.
enum class ClockType : clockid_t {
	/// System-wide wall clock time, settable.
	REALTIME = CLOCK_REALTIME,
	/// A faster but less precise version of REALTIME, not settable.
	REALTIME_COARSE = CLOCK_REALTIME_COARSE,
	/// System-wide wall clock time based on international atomic time (TAI) - it is ignoring leap seconds.
	ATOMIC_REALTIME = CLOCK_TAI,
	/// System-wide clock representing monotonic time since some unspecified point in the past.
	/**
	 * On Linux this corresponds to the time since the system was started.
	 **/
	MONOTONIC = CLOCK_MONOTONIC,
	/// Like MONOTONIC but not affected by NTP adjustments.
	MONOTONIC_RAW = CLOCK_MONOTONIC_RAW,
	/// A faster but less precise version of MONOTONIC, does not count suspend time.
	MONOTONIC_COARSE = CLOCK_MONOTONIC_COARSE,
	/// Like MONOTONIC but also counts suspend time.
	BOOTTIME = CLOCK_BOOTTIME,
	/// Counts the CPU time consumed by the calling process.
	PROCESS_CPUTIME = CLOCK_PROCESS_CPUTIME_ID,
	/// Counts the CPU time consumed by the calling thread.
	THREAD_CPUTIME = CLOCK_THREAD_CPUTIME_ID,
	INVALID = clockid_t{-1}
};

/// Base class for all clock types.
class COSMOS_API ClockBase {
protected:
	void nowFromClock(TimeSpec &ts, ClockType clock) const;
	void sleepOnClock(const TimeSpec until, ClockType clock) const;
};

/// C++ wrapper around the POSIX clocks and related functions.
/**
 * This is a template to make different clock types incompatible with each
 * other.
 **/
template <ClockType CLOCK>
class Clock : public ClockBase {
public: // functions

	/// Retrieve the current value of the clock.
	void now(TimeSpec &ts) const {
		return this->nowFromClock(ts, CLOCK);
	}

	/// Returns the current value of the clock by value.
	TimeSpec now() const {
		TimeSpec ret;
		now(ret);
		return ret;
	}

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
	void sleep(const TimeSpec until) {
		sleepOnClock(until, CLOCK);
	}

	static ClockType raw() { return CLOCK; }
};

using MonotonicClock = Clock<ClockType::MONOTONIC>;
using RealtimeClock = Clock<ClockType::REALTIME>;

} // end ns

#endif // inc. guard
