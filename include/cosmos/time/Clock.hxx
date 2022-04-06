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

/**
 * \brief
 *	A C++ wrapper around the POSIX clocks and related functions
 **/
class COSMOS_API Clock {
public: // functions

	explicit Clock(const ClockType &type) :
		m_type(type)
	{}

	void now(TimeSpec &ts) const;

	TimeSpec now() const {
		TimeSpec ret;
		now(ret);
		return ret;
	}

	clockid_t rawType() const { return static_cast<clockid_t>(m_type); }

protected: // data

	//! the clock type to operate on
	const ClockType m_type = ClockType::INVALID;
};

/**
 * \brief
 * 	A type to measure elapsed time based on a given clock type
 **/
class StopWatch {
public: // functions

	explicit StopWatch(const ClockType &type) :
		m_clock(type)
	{}

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
	Clock m_clock;
};

} // end ns

#endif // inc. guard
