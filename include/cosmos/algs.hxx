#ifndef COSMOS_ALGS_HXX
#define COSMOS_ALGS_HXX

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

}

#endif // inc. guard
