#pragma once

// cosmos
#include <cosmos/error/RuntimeError.hxx>
#include <cosmos/net/InterfaceAddress.hxx>

namespace cosmos {

/// Iterator helper type for InterfaceAddressList.
/**
 * The `struct ifaddrs*` stored in InterfaceAddressList is a linked list. This
 * iterator type walks through this list. This is a simple InputIterator type.
 **/
class InterfaceAddressIterator {
	friend class InterfaceAddressList;
protected: // functions

	explicit InterfaceAddressIterator(struct ifaddrs *pos) :
			m_pos{pos} {
	}

public: // functions

	auto& operator++() {
		if (m_pos) {
			m_pos = m_pos->ifa_next;
		} else {
			throw RuntimeError{"Attempt to increment InterfaceAddressIterator past the end"};
		}
		return *this;
	}

	InterfaceAddress operator*() {
		if (!m_pos) {
			throw RuntimeError{"Attempt to dereference invalid InterfaceAddressIterator"};
		}

		return InterfaceAddress{m_pos};
	}

	bool operator==(const InterfaceAddressIterator &other) const {
		return m_pos == other.m_pos;
	}

	bool operator!=(const InterfaceAddressIterator &other) const {
		return !(*this == other);
	}

protected: // data

	struct ifaddrs *m_pos = nullptr;
};

} // end ns
