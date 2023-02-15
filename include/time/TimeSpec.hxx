#ifndef COSMOS_TIMESPEC_HXX
#define COSMOS_TIMESPEC_HXX

// C++
#include <chrono>

// Linux
#include <time.h>

namespace cosmos {

/// A C++ wrapper around the POSIX struct timespec
/**
 * TODO: The TimeSpec could also be coupled to the clock type it is compatible
 * with, thereby gaining type safety also on this level, it would not be
 * possible to use a TimeSpec from the wrong clock with an API.
 **/
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

	/// Deliberately don't initialize the members for performance reasons
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
		this->tv_sec = ns.count() / nanosecondBase();
		this->tv_nsec = (ns.count() % nanosecondBase());
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
			ret.tv_nsec += nanosecondBase();
		}

		return ret;
	}

	TimeSpec operator+(const TimeSpec &other) const {
		TimeSpec ret;

		ret.tv_sec = this->tv_sec + other.tv_sec;
		ret.tv_nsec = this->tv_nsec + other.tv_nsec;

		if (ret.tv_nsec >= nanosecondBase()) {
			++ret.tv_sec;
			ret.tv_nsec -= nanosecondBase();
		}

		return ret;
	}

protected: // functions

	constexpr long nanosecondBase() const {
		return 1000 * 1000 * 1000;
	}
};

} // end ns

#endif // inc. guard
