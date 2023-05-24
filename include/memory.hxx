#ifndef COSMOS_MEMORY_HXX
#define COSMOS_MEMORY_HXX

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
 **/
template <typename T>
void zero_object(T &obj) {
	// this should only be done for non-C++ types
	static_assert(std::is_trivial<T>::value == true);
	std::memset(&obj, 0, sizeof(T));
}

} // end ns

#endif // inc. guard
