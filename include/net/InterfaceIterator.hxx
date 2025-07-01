#pragma once

// cosmos
#include <cosmos/error/RuntimeError.hxx>
#include <cosmos/net/InterfaceInfo.hxx>

namespace cosmos {

/// Helper class to iterate over InterfaceEnumerator.
/**
 * InterfaceEnumerator holds a list of structures allocated in libc. This
 * iterator is able to iterate over the list. Usually you don't need to use it
 * explicitly, a range based for loop used on InterfaceEnumerator will use it
 * automatically.
 *
 * The end of the array is marked by a nullptr if_name and zero if_index. We
 * keep such an element in the M_END member, which serves as the end()
 * iterator in InterfaceEnumerator.
 **/
class InterfaceIterator {
public: // functions

	constexpr InterfaceIterator() {}

	InterfaceIterator(const InterfaceInfo *pos) :
		m_pos{pos ? pos : &M_END}
	{}

	auto& operator++() {
		if (m_pos && m_pos->if_name != nullptr) {
			m_pos++;
		} else {
			throw RuntimeError{"Attempt to increment InterfaceIterator past the end"};
		}

		return *this;
	}

	const InterfaceInfo& operator*() {
		if (!m_pos || m_pos->if_name == nullptr) {
			throw RuntimeError{"Attempt to dereference invalid InterfaceIterator"};
		}

		return *m_pos;
	}

	bool operator==(const InterfaceIterator &other) const {
		if (m_pos == other.m_pos)
			return true;
		else if (!m_pos || !other.m_pos)
			return false;

		return m_pos->if_name == other.m_pos->if_name &&
			m_pos->if_index == other.m_pos->if_index;
	}

	bool operator!=(const InterfaceIterator &other) const {
		return !(*this == other);
	}

protected: // data

	static constexpr InterfaceInfo M_END = InterfaceInfo{};

	const InterfaceInfo *m_pos = &M_END;
};

} // end ns
