#ifndef COSMOS_CLOCK_HXX
#define COSMOS_CLOCK_HXX

// Linux
#include <time.h>

// cosmos
#include "cosmos/time/TimeSpec.hxx"
#include "cosmos/types.hxx"

namespace cosmos {

/// Available clock types
enum class ClockType : clockid_t {
	/// System-wide wall clock team, settable
	REALTIME = CLOCK_REALTIME,
	/// A faster but less precise version of REALTIME, not settable
	REALTIME_COARSE = CLOCK_REALTIME_COARSE,
	/// System-wide clock representing monotonic time since some unspecified point in the past
	/**
	 * On Linux this corresponds to the time since the system was started.
	 **/
	MONOTONIC = CLOCK_MONOTONIC,
	/// Like MONOTONIC but not affected by NTP adjustments
	MONOTONIC_RAW = CLOCK_MONOTONIC_RAW,
	/// A faster but less precise version of MONOTONIC, does not count suspend time
	MONOTONIC_COARSE = CLOCK_MONOTONIC_COARSE,
	/// Like MONOTONIC but also counts suspend time
	BOOTTIME = CLOCK_BOOTTIME,
	/// Counts the CPU time consumed by the calling process
	PROCESS_CPUTIME = CLOCK_PROCESS_CPUTIME_ID,
	/// Counts the CPU time consumed by the calling thread
	THREAD_CPUTIME = CLOCK_THREAD_CPUTIME_ID,
	INVALID
};

/// Base class for all clock types
class COSMOS_API ClockBase {
protected:
	void nowFromClock(TimeSpec &ts, ClockType clock) const;
};

/// C++ wrapper around the POSIX clocks and related functions
template <ClockType CLOCK>
class Clock : public ClockBase {
public: // functions

	/// Retrieve the current value of the clock
	void now(TimeSpec &ts) const {
		return this->nowFromClock(ts, CLOCK);
	}

	TimeSpec now() const {
		TimeSpec ret;
		now(ret);
		return ret;
	}

	static clockid_t rawType() { return static_cast<clockid_t>(CLOCK); }
};

using MonotonicClock = Clock<ClockType::MONOTONIC>;
using RealtimeClock = Clock<ClockType::REALTIME>;

/// A type to measure elapsed time based on a given clock type
template <ClockType CLOCK>
class StopWatch {
public: // types

	using InitialMark = NamedBool<struct mark_t, false>;

public: // functions

	/// Construct and optionally set an initial mark()
	explicit StopWatch(const InitialMark do_mark = InitialMark()) {
		if (do_mark)
			mark();
	}

	/// Set a new stop mark to compare against
	void mark() {
		m_clock.now(m_mark);
	}

	/// Returns the elapsed milliseconds since the active mark
	size_t elapsedMs() const {
		return (m_clock.now() - m_mark).toMilliseconds();
	}

	std::chrono::milliseconds elapsed() const {
		return static_cast<std::chrono::milliseconds>(m_clock.now() - m_mark);
	}

	/// Returns the currently set mark (undefined if mark() was never called!)
	TimeSpec currentMark() const {
		return m_mark;
	}

protected: // data

	TimeSpec m_mark;
	Clock<CLOCK> m_clock;
};

using MonotonicStopWatch = StopWatch<ClockType::MONOTONIC>;
using RealtimeStopWatch = StopWatch<ClockType::REALTIME>;

} // end ns

#endif // inc. guard
