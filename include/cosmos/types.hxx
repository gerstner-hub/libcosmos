#ifndef COSMOS_TYPES_HXX
#define COSMOS_TYPES_HXX

// C++ stdlib
#include <functional>
#include <map>
#include <ostream>
#include <string>
#include <vector>

namespace cosmos {

/*
 * some general types used across cosmos
 */

typedef std::vector<std::string> StringVector;
typedef std::vector<const char*> CStringVector;

/// Strong template type to wrap boolean values in a named type
/**
 * This type is intended as a replacement for primitive bool values in for
 * constructor and function arguments. The purpose is to increase readability
 * and avoid programming mistakes by passing a bool value for something else
 * than intended.
 *
 * In the absence of named parameter passing in C++ like `MyObj(do_this=true)`
 * one can use a specialization of this template type: `MyObj(DoThis(true))`.
 *
 * To use it you need to define an arbitrary tag type and the default boolean
 * value to apply like:
 *
 *     class my_setting_t {};
 *     using MySetting = NamedBool<my_setting_t, true>;
 *
 *     void myfunc(const MySetting &setting = MySetting());
 *
 *     // will be called with a true default value
 *     myfunc();
 *     // will be called with a false value
 *     myfunc(MySetting(false));
 *
 * By providing the bool cast operator the type should behave mostly like a
 * regular bool when querying its value.
 **/
template <typename _, bool def>
class NamedBool {
public:
	explicit NamedBool(bool val=def) :
		m_val(val) {}

	operator bool() const { return m_val; }
protected:
	bool m_val = def;
};

/// Helper class to guard arbitrary resources
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
		m_res(r),
		m_cleaner(cleaner)
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

} // end ns

/**
 * \brief
 * 	Output all the elements of a vector as a comma separated list
 **/
template <typename T>
inline std::ostream& operator<<(std::ostream &o, const std::vector<T> &sv) {
	for (auto it = sv.begin(); it != sv.end(); it++) {
		if (it != sv.begin())
			o << ", ";
		o << *it;
	}

	return o;
}

/**
 * \brief
 * 	Output all the elements of a map as a "key:value" newline separated
 * 	list
 **/
template <typename K, typename V>
inline std::ostream& operator<<(std::ostream &o, const std::map<K,V> &m) {
	for (auto it: m) {
		o << it.first << ": " << it.second << "\n";
	}

	return o;
}

#endif // inc. guard
