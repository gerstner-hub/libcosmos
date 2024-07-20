#pragma once

// C++
#include <cstring>
#include <type_traits>

/**
 * @file
 *
 * Helper functions for dealing with raw memory.
 **/

namespace cosmos {

/// Completely overwrites the given object with zeroes.
/**
 * This is typically used in C-APIs to get a defined object state.
 *
 * This is _not_ an optimization-safe function to remove sensitive data from
 * memory.
 **/
template <typename T>
void zero_object(T &obj) {
	// this should only be done for non-C++ types
	static_assert(std::is_trivial<T>::value == true);
	std::memset(&obj, 0, sizeof(T));
}

} // end ns
