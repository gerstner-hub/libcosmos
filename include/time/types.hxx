#ifndef COSMOS_TIME_TYPES_HXX
#define COSMOS_TIME_TYPES_HXX

// Linux
#include <time.h>

// C++
#include <chrono>

/**
 * @file
 *
 * Basic time related type definitions.
 **/

namespace cosmos {

/// Available clock types for time operations.
enum class ClockType : clockid_t {
	/// System-wide wall clock time, settable.
	REALTIME         = CLOCK_REALTIME,
	/// A faster but less precise version of REALTIME, not settable.
	REALTIME_COARSE  = CLOCK_REALTIME_COARSE,
	/// System-wide wall clock time based on international atomic time (TAI) - it is ignoring leap seconds.
	ATOMIC_REALTIME  = CLOCK_TAI,
	/// System-wide clock representing monotonic time since some unspecified point in the past.
	/**
	 * On Linux this corresponds to the time since the system was started.
	 **/
	MONOTONIC        = CLOCK_MONOTONIC,
	/// Like MONOTONIC but not affected by NTP adjustments.
	MONOTONIC_RAW    = CLOCK_MONOTONIC_RAW,
	/// A faster but less precise version of MONOTONIC, does not count suspend time.
	MONOTONIC_COARSE = CLOCK_MONOTONIC_COARSE,
	/// Like MONOTONIC but also counts suspend time.
	BOOTTIME         = CLOCK_BOOTTIME,
	/// Counts the CPU time consumed by the calling process.
	PROCESS_CPUTIME  = CLOCK_PROCESS_CPUTIME_ID,
	/// Counts the CPU time consumed by the calling thread.
	THREAD_CPUTIME   = CLOCK_THREAD_CPUTIME_ID,
	INVALID          = clockid_t{-1}
};

/// A C++ wrapper around the POSIX struct timespec coupled to a specific CLOCK type.
template <ClockType CLOCK>
class TimeSpec :
		public timespec {
public:
	explicit TimeSpec(time_t seconds, long nano_seconds = 0) {
		this->tv_sec = seconds;
		this->tv_nsec = nano_seconds;
	}

	explicit TimeSpec(const std::chrono::milliseconds ms) {
		set(ms);
	}

	explicit TimeSpec(const std::chrono::nanoseconds ns) {
		set(ns);
	}

	/// Deliberately don't initialize the members for performance reasons.
	TimeSpec() {}

	bool isZero() const { return this->tv_sec == 0 && this->tv_nsec == 0; }
	void reset() { this->tv_sec = 0; this->tv_nsec = 0; }

	time_t getSeconds() const { return this->tv_sec; }
	long getNanoseconds() const { return this->tv_nsec; }

	void setSeconds(const time_t seconds) { this->tv_sec = seconds; }
	void setNanoseconds(const long nano_seconds) { this->tv_nsec = nano_seconds; }

	void addSeconds(const time_t seconds) {
		this->tv_sec += seconds;
	}

	void addNanoseconds(const long nano_seconds) {
		this->tv_nsec += nano_seconds;
	}

	TimeSpec& setAsMilliseconds(const size_t milliseconds) {
		auto left_ms = milliseconds % 1000;
		this->tv_sec = (milliseconds - left_ms) / 1000;
		this->tv_nsec = left_ms * 1000 * 1000;
		return *this;
	}

	TimeSpec& set(const std::chrono::milliseconds ms) {
		this->tv_sec = ms.count() / 1000;
		this->tv_nsec = (ms.count() % 1000) * 1000 * 1000;
		return *this;
	}

	TimeSpec& set(const std::chrono::nanoseconds ns) {
		this->tv_sec = ns.count() / NANOSECOND_BASE;
		this->tv_nsec = (ns.count() % NANOSECOND_BASE);
		return *this;
	}

	/// Converts the time representation into a single milliseconds value
	size_t toMilliseconds() const {
		size_t ret = this->tv_sec * 1000;
		ret += (this->tv_nsec / 1000 / 1000);
		return ret;
	}

	explicit operator std::chrono::milliseconds() const {
		return std::chrono::milliseconds{toMilliseconds()};
	}

	bool operator<(const TimeSpec &other) const {
		if (this->tv_sec < other.tv_sec)
			return true;
		else if (this->tv_sec != other.tv_sec)
			return false;

		// so seconds are equal
		if (this->tv_nsec < other.tv_nsec)
			return true;

		return false;
	}

	bool operator>=(const TimeSpec &other) const {
		return !operator<(other);
	}

	bool operator==(const TimeSpec &other) const {
		return this->tv_sec == other.tv_sec &&
			this->tv_nsec == other.tv_nsec;
	}

	bool operator!=(const TimeSpec &other) const { return !(*this == other); }

	bool operator<=(const TimeSpec &other) const {
		return *this < other || *this == other;
	}

	TimeSpec operator-(const TimeSpec &other) const {
		TimeSpec ret;

		ret.tv_sec = this->tv_sec - other.tv_sec;
		ret.tv_nsec = this->tv_nsec - other.tv_nsec;

		if (ret.tv_nsec < 0) {
			--ret.tv_sec;
			ret.tv_nsec += NANOSECOND_BASE;
		}

		return ret;
	}

	TimeSpec operator+(const TimeSpec &other) const {
		TimeSpec ret;

		ret.tv_sec = this->tv_sec + other.tv_sec;
		ret.tv_nsec = this->tv_nsec + other.tv_nsec;

		if (ret.tv_nsec >= NANOSECOND_BASE) {
			++ret.tv_sec;
			ret.tv_nsec -= NANOSECOND_BASE;
		}

		return ret;
	}

protected: // functions

	static constexpr long NANOSECOND_BASE{1000 * 1000 * 1000};
};

typedef TimeSpec<ClockType::ATOMIC_REALTIME>  AtomicRealTime;
typedef TimeSpec<ClockType::BOOTTIME>         BootTime;
typedef TimeSpec<ClockType::MONOTONIC_COARSE> CoarseMonotonicTime;
typedef TimeSpec<ClockType::MONOTONIC>        MonotonicTime;
typedef TimeSpec<ClockType::MONOTONIC_RAW>    RawMonotonicTime;
typedef TimeSpec<ClockType::PROCESS_CPUTIME>  ProcessCpuTime;
typedef TimeSpec<ClockType::REALTIME_COARSE>  CoarseRealTime;
typedef TimeSpec<ClockType::REALTIME>         RealTime;
typedef TimeSpec<ClockType::THREAD_CPUTIME>   ThreadCpuTime;

} // end ns

#endif // inc. guard
