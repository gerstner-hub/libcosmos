#ifndef COSMOS_ALGS_HXX
#define COSMOS_ALGS_HXX

namespace cosmos {

/// checks whether \c is within the given range, inclusive
// allow the ranger type to differ from the compare type to avoid issues with
// literal numbers comparing against compatible integer variables. The range
// types should match each other, though
template <typename T1, typename T2>
bool in_range(const T1 &v, const T2 &_min, const T2 &_max) {
	return _min <= v && v <= _max;
}

}

#endif // inc. guard
