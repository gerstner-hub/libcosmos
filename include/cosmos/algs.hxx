#ifndef COSMOS_ALGS_HXX
#define COSMOS_ALGS_HXX

// C++
#include <cstddef>
#include <type_traits>
#include <vector>

namespace cosmos {

/**
 * @file
 *
 * This header contains some helper algorithm-like functions for dealing with
 * STL containers, types and more.
 **/

template<typename T>
struct identity { typedef T type; };

template<typename T>
using identity_t = typename identity<T>::type;

/// Checks whether \c is within the given range, inclusive
// the stunt with identity_t is required to avoid deduction problems when e.g.
// literal integer constants are involved.
template <typename T1>
bool in_range(const T1 &v, const identity_t<T1> &_min, const identity_t<T1> &_max) {
	return _min <= v && v <= _max;
}

/// Checks whether the value \c v is found in the given list of values \c l
template <typename T>
bool in_list(const T &v, const std::initializer_list<T> &l) {
	for (const auto &cmp: l) {
		if (v == cmp)
			return true;
	}

	return false;
}

/// Returns the number of elements in a C style array
template <typename T>
constexpr size_t num_elements(const T &v) {
	return sizeof(v) / sizeof(v[0]);
}

/// Append iterable sequence v2 to sequence v1
template <typename T1, typename T2>
T1& append(T1 &v1, const T2 &v2) {
	v1.insert(std::end(v1), std::begin(v2), std::end(v2));
	return v1;
}

/// Casts an enum constant value into its underlying primitive type
template<typename ENUM>
constexpr auto to_integral(const ENUM e) -> typename std::underlying_type<ENUM>::type {
   return static_cast<typename std::underlying_type<ENUM>::type>(e);
}

} // end ns

#endif // inc. guard
