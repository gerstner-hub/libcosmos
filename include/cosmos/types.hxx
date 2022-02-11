#ifndef COSMOS_TYPES_HXX
#define COSMOS_TYPES_HXX

// C++ stdlib
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
