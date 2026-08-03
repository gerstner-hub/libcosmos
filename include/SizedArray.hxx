#pragma once

// C++
#include <array>
#include <cstddef>
#include <stdexcept>

namespace cosmos {

/// Wrapper around std::array which adds a size of currently used elements.
/**
 * std::array is an efficient type when the number of elements required is
 * known at compile-time, or when only a small amount of elements will ever be
 * required in performance-critical contexts. For the latter case std:array is
 * lacking a "used element" count, however, which makes it hard to deal with
 * actually present elements.
 *
 * SizedArray adds exactly that, std::vector-like semantics up to the maximum
 * amount of N elements.
 **/
template <typename T, std::size_t N>
class SizedArray {
	static_assert(N > 1, "SizedArray with <= 1 elements makes no sense");
public: // types

	using Array = std::array<T, N>;
	using iterator = Array::iterator;
	using const_iterator = Array::const_iterator;
	using reverse_iterator = Array::reverse_iterator;
	using const_reverse_iterator = Array::const_reverse_iterator;
	using reference = Array::reference;
	using const_reference = Array::const_reference;

public: // functions

	/// Initializes `size` elements with the value `init`
	explicit SizedArray(const T &init = {}, const size_t size = 0) :
			m_used{size} {
		if (size > N) {
			throw std::out_of_range{"excess array init size"};
		}

		m_array.fill(init);
	}

	void clear() {
		m_used = 0;
	}

	bool empty() const {
		return m_used == 0;
	}

	size_t max_size() const {
		return N;
	}

	size_t size() const {
		return m_used;
	}

	T* data() noexcept {
		return m_array.data();
	}

	const T* data() const noexcept {
		return m_array.data();
	}

	T& operator[](const size_t i) {
		return m_array[i];
	}

	const T& operator[](const size_t i) const {
		return m_array[i];
	}

	reference at(const size_t i) {
		if (i >= m_used) {
			throw std::out_of_range{"access beyond used elements"};
		}

		return m_array.at(i);
	}

	const_reference at(const size_t i) const {
		if (i >= m_used) {
			throw std::out_of_range{"access beyond used elements"};
		}

		return m_array.at(i);
	}

	void push_back(const T &t) {
		if (m_used == N) {
			throw std::bad_alloc();
		}

		m_array[m_used++] = t;
	}

	void pop_back() {
		if (!m_used) {
			throw std::runtime_error("pop on empty array");
		}

		m_used--;
	}

	reference front() {
		return m_array.front();
	}

	const_reference front() const {
		return m_array.front();
	}

	reference back() {
		return *rbegin();
	}

	const_reference back() const {
		return *rbegin();
	}

	SizedArray operator=(const SizedArray &other) {
		m_used = other.m_used;

		for (size_t i = 0; i < m_used; i++) {
			m_array[i] = other.m_array[i];
		}
	}

	iterator begin() {
		return m_array.begin();
	}

	const_iterator begin() const {
		return m_array.begin();
	}

	const_iterator cbegin() const {
		return begin();
	}


	iterator end() {
		return m_array.begin() + m_used;
	}

	const_iterator end() const {
		return m_array.begin() + m_used;
	}

	const_iterator cend() const {
		return end();
	}


	reverse_iterator rbegin() {
		return m_array.rbegin() + N - m_used;
	}

	const_reverse_iterator rbegin() const {
		return m_array.rbegin() + N - m_used;
	}

	const_reverse_iterator crbegin() const {
		return rbegin();
	}


	reverse_iterator rend() {
		return m_array.rend();
	}

	const_reverse_iterator rend() const {
		return m_array.rend();
	}

	const_reverse_iterator crend() const {
		return rend();
	}

protected: // data

	Array m_array;
	size_t m_used = 0;
};

} // end ns
