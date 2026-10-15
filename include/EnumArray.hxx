// C++
#include <array>
#include <type_traits>

namespace cosmos {

/// std::array with enum-based index access.
/**
 * This is a small wrapper around std::array, offering access to individual
 * members via an `enum class` type representing specific indices. `INDEX_T`
 * should be an enum class type based on `std::size_t`. You can then access
 * individual elements in a manner like this:
 *
 * enum_array[Index::MyIndex] = 10;
 *
 * The more common member functions are wrapped by this. For direct access to
 * the unerlying array use the `plain()` member functions.
 **/
template <typename INDEX_T, typename VAL_T, std::size_t N>
class EnumArray {
	static_assert(std::is_enum_v<INDEX_T>, "non-enum index type encountered");

public: // types

	using RawArray = std::array<VAL_T, N>;

public: // functions

	VAL_T& operator[](const INDEX_T index) {
		return m_array[size_t(index)];
	}

	const VAL_T& operator[](const INDEX_T index) const {
		return m_array[size_t(index)];
	}

	auto size() const {
		return m_array.size();
	}

	auto begin() {
		return m_array.begin();
	}

	auto end() {
		return m_array.end();
	}

	auto begin() const {
		return m_array.cbegin();
	}

	auto end() const {
		return m_array.cend();
	}

	auto rbegin() {
		return m_array.rbegin();
	}

	auto rend() {
		return m_array.rend();
	}

	auto& plain() {
		return m_array;
	}

	const auto& plain() const {
		return m_array;
	}

	auto empty() const {
		return m_array.empty();
	}

protected: // data

	RawArray m_array;
};

} // end ns
