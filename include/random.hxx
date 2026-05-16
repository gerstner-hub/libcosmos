#pragma once

// C++
#include <cstdint>
#include <vector>

// Linux
#include <sys/random.h>

// cosmos
#include <cosmos/BitMask.hxx>

namespace cosmos {

/// Flag type used with `cosmos::get_random()`.
/**
 * 
 **/
enum class GetRandomFlag : int {
	FROM_RANDOM = GRND_RANDOM,  ///< Obtain data from the random device instead of the urandom device.
	NONBLOCK    = GRND_NONBLOCK ///< Don't block waiting for random data, rather fail with Errno::AGAIN.
};

/// Flags bit mask used with `cosmos::get_random()`.
using GetRandomFlags = BitMask<GetRandomFlag>;

/// Request random data from the kernel.
/**
 * This is a low-level function call to obtain random data from the kernel.
 * `amount` bytes of random data will be requested to be placed into `buffer`.
 * The amount of data placed into the buffer can be shorter than requested if
 * the call is interrupted or insufficient entropy is available in the system
 * and GetRandomFlag::NONBLOCK is set..
 *
 * On error an ApiError containing one of the following Errnos is thrown:
 *
 * - Errno::AGAIN: insufficient random data was available and
 *   GetRandomFlag::NONBLOCK was set.
 * - Errno::FAULT: `buffer` points outside addressable address space.
 * - Errno::INVALID_ARG: bad value(s) in `flags`.
 * - Errno::INTERRUPTED: the system call was interrupted by a signal handler
 *   and transparent restart is not configured via
 *   `set_restart_syscall_on_interrupt()`.
 **/
size_t COSMOS_API get_random(void *buffer, const size_t amount, const GetRandomFlags flags = {});

/// Obtain random data in a `std::vector`.
/**
 * This is a convenience overload of `get_random(void*, const size_t, const
 * GetRandomFlags)`. It returns an STL vector containing the random data. In
 * case not all `amount` bytes could be provided by the kernel then the
 * returned vector will contain less data than requested, accordingly.
 **/
inline std::vector<uint8_t> get_random(size_t amount, const GetRandomFlags flags = {}) {
	std::vector<uint8_t> ret(amount);
	amount = get_random(ret.data(), ret.size(), flags);
	ret.resize(amount);
	return ret;
}

} // end ns
