#ifndef COSMOS_CLOCK_HXX
#define COSMOS_CLOCK_HXX

// Linux
#include <time.h>

// cosmos
#include "cosmos/time/TimeSpec.hxx"

namespace cosmos {

//! available clock types
enum class ClockType : clockid_t {
	REALTIME = CLOCK_REALTIME,
	REALTIME_COARSE = CLOCK_REALTIME_COARSE,
	MONOTONIC = CLOCK_MONOTONIC,
	MONOTONIC_RAW = CLOCK_MONOTONIC_RAW,
	BOOTTIME = CLOCK_BOOTTIME,
	PROCESS_CPUTIME = CLOCK_PROCESS_CPUTIME_ID,
	THREAD_CPUTIME = CLOCK_THREAD_CPUTIME_ID,
	INVALID
};

class COSMOS_API ClockBase {
protected:
	void nowFromClock(TimeSpec &ts, ClockType clock) const;
};

/**
 * \brief
 *	A C++ wrapper around the POSIX clocks and related functions
 **/
template <ClockType CLOCK>
class Clock : public ClockBase {
public: // functions

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

/**
 * \brief
 * 	A type to measure elapsed time based on a given clock type
 **/
template <ClockType CLOCK>
class StopWatch {
public: // functions

	void mark() {
		m_clock.now(m_mark);
	}

	size_t elapsedMs() const {
		return (m_clock.now() - m_mark).toMilliseconds();
	}

	std::chrono::milliseconds elapsed() const {
		return static_cast<std::chrono::milliseconds>(m_clock.now() - m_mark);
	}

protected: // data

	TimeSpec m_mark;
	Clock<CLOCK> m_clock;
};

using MonotonicStopWatch = StopWatch<ClockType::MONOTONIC>;
using RealtimeStopWatch = StopWatch<ClockType::REALTIME>;

} // end ns

#endif // inc. guard
