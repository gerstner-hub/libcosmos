#pragma once

// C++
#include <cstddef>
#include <functional>
#include <map>
#include <ostream>
#include <type_traits>
#include <variant>
#include <vector>

namespace cosmos {

/**
 * @file
 *
 * This header contains some helper algorithm-like functions for dealing with
 * STL containers, types and more.
 *
 * Furthermore some general convenience types used across libcosmos.
 **/

/// Strong template type to wrap boolean values in a named type.
/**
 * This type is intended as a replacement for primitive bool values for
 * constructor and function arguments. The purpose is to increase readability
 * and avoid programming mistakes by passing a bool value for something else
 * than intended.
 *
 * Since right now there is no named parameter passing in C++ like
 * `MyObj(do_this=true)` one can use a specialization of this template type:
 * `MyObj(DoThis(true))`.
 *
 * To use it you need to define an arbitrary tag type and the default boolean
 * value to apply like:
 *
 *     using MySetting = NamedBool<struct my_setting_t, true>;
 *
 *     void myfunc(const MySetting setting = MySetting());
 *
 *     // will be called with a true default value
 *     myfunc();
 *     // will be called with a false value
 *     myfunc(MySetting(false));
 *
 * By providing the bool cast operator the type wille behave like a regular
 * bool when querying its value.
 **/
template <typename _, bool def>
class NamedBool {
public:
	explicit NamedBool(bool val=def) :
		m_val{val} {}

	operator bool() const { return m_val; }

	void flip() {
		m_val = !m_val;
	}
protected:
	bool m_val = def;
};

/// Helper class to guard arbitrary resources.
/**
 * For non-heap resources (for which stdlib smart pointers should be used)
 * a specialization of this type can be used which takes a custom cleanup
 * function to be run during destruction.
 **/
template <typename R>
class ResourceGuard {
public: // types

	using CleanFunc = void (R);

public: // functions

	ResourceGuard(const ResourceGuard &) = delete;
	ResourceGuard& operator=(const ResourceGuard &) = delete;

	ResourceGuard(R r, std::function<CleanFunc> cleaner) :
		m_res{r},
		m_cleaner{cleaner}
	{}

	~ResourceGuard() {
		if (!m_disarmed) {
			m_cleaner(m_res);
		}
	}

	void disarm() { m_disarmed = true; }

protected: // data

	bool m_disarmed = false;
	R m_res;
	std::function<CleanFunc> m_cleaner;
};

template<typename T>
struct Identity { using type = T; };

template<typename T>
using IdentityT = typename Identity<T>::type;

/// Checks whether \c is within the given (inclusive) range.
// the stunt with IdentityT is required to avoid deduction problems when e.g.
// literal integer constants are involved.
template <typename T1>
bool in_range(const T1 &v, const IdentityT<T1> &_min, const IdentityT<T1> &_max) {
	return _min <= v && v <= _max;
}

/// Checks whether the value \c v is found in the given list of values \c l.
template <typename T>
bool in_list(const T &v, const std::initializer_list<T> &l) {
	for (const auto &cmp: l) {
		if (v == cmp)
			return true;
	}

	return false;
}

/// Returns the number of elements in a C style array.
template <typename T>
constexpr size_t num_elements(const T &v) {
	return sizeof(v) / sizeof(v[0]);
}

/// Append iterable sequence v2 to sequence v1.
template <typename T1, typename T2>
T1& append(T1 &v1, const T2 &v2) {
	v1.insert(std::end(v1), std::begin(v2), std::end(v2));
	return v1;
}

/// Casts an enum constant value into its underlying primitive type.
template<typename ENUM>
constexpr auto to_integral(const ENUM e) -> typename std::underlying_type<ENUM>::type {
	return static_cast<typename std::underlying_type<ENUM>::type>(e);
}

/// Returns a pointer casted to the underlying type of the given enum.
template<typename ENUM>
auto to_raw_ptr(ENUM *e) {
	using UT = typename std::underlying_type<ENUM>::type;
	return reinterpret_cast<UT*>(e);
}

template <typename VARIANT>
bool is_empty_variant(const VARIANT &var) {
	return std::holds_alternative<std::monostate>(var);
}

template <typename VARIANT>
bool variant_holds_value(const VARIANT &var) {
	return !is_empty_variant(var);
}

} // end ns

/// Output all the elements of a vector as a comma separated list.
template <typename T>
inline std::ostream& operator<<(std::ostream &o, const std::vector<T> &sv) {
	for (auto it = sv.begin(); it != sv.end(); it++) {
		if (it != sv.begin())
			o << ", ";
		o << *it;
	}

	return o;
}

/// Output all the elements of a map as a "key:value" newline separated list.
template <typename K, typename V>
inline std::ostream& operator<<(std::ostream &o, const std::map<K,V> &m) {
	for (auto it: m) {
		o << it.first << ": " << it.second << "\n";
	}

	return o;
}
