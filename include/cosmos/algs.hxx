#ifndef COSMOS_ALGS_HXX
#define COSMOS_ALGS_HXX

#include <vector>

namespace cosmos {

template<typename T>
struct identity { typedef T type; };

template<typename T>
using identity_t = typename identity<T>::type;

/// checks whether \c is within the given range, inclusive
// the stunt with identity_t is required to avoid deduction problems when e.g.
// literal integer constants are involved.
template <typename T1>
bool in_range(const T1 &v, const identity_t<T1> &_min, const identity_t<T1> &_max) {
	return _min <= v && v <= _max;
}

/// returns the number of elements in a C style array
template <typename T>
constexpr size_t num_elements(const T &v) {
	return sizeof(v) / sizeof(v[0]);
}

/// append sequence v2 to sequence v1
template <typename T1, typename T2>
T1& append(T1 &v1, const T2 &v2) {
	v1.insert(std::end(v1), std::begin(v2), std::end(v2));
	return v1;
}

}

#endif // inc. guard
