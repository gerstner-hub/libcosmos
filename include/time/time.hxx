#pragma once

// C++
#include <chrono>

// cosmos
#include <cosmos/time/Clock.hxx>

/**
 * @file
 *
 * This header contains global time related functions.
 **/

namespace cosmos::time {

/// Suspends execution of the calling thread for the given number of nanoseconds.
/**
 * This takes a relative sleep duration. If the call is interrupted and
 * automatic system call restarting is not enabled in libcosmos then
 * continuing the sleep will be difficult, because it is unknown how much
 * sleep duration is left.
 *
 * Use ClockBase::sleep() directly using an absolute sleep time to avoid this
 * situation.
 *
 * This sleep() is based on the monotonic clock.
 **/
inline void sleep(std::chrono::nanoseconds ns) {
	auto clock = MonotonicClock{};
	auto now = clock.now();
	clock.sleep(now + MonotonicTime{ns});
}

/// \see sleep(std::chrono::nanoseconds)
inline void sleep(std::chrono::microseconds us) {
	return sleep(std::chrono::nanoseconds{us});
}

/// \see sleep(std::chrono::nanoseconds)
inline void sleep(std::chrono::milliseconds ms) {
	return sleep(std::chrono::nanoseconds{ms});
}

} // end ns
