#pragma once

// cosmos
#include "cosmos/error/RuntimeError.hxx"
#include "cosmos/net/AddressInfo.hxx"

namespace cosmos {

/// Helper class to iterate over AddressInfoList.
/**
 * AddressInfoList holds a list of AddressInfo structs allocated in libc. This
 * type is able to iterate over the list. Usually you don't need to use it
 * explicitly, a range based for loop used on AddressInfoList will use it
 * automatically.
 *
 * The end of the list is marked by a nullptr ai_next field in AddressInfo.
 **/
struct AddressInfoIterator {
public: // functions

	AddressInfoIterator() {}

	explicit AddressInfoIterator(const AddressInfo *pos) :
		m_pos{pos}
	{}

	auto& operator++() {
		if (!m_pos) {
			cosmos_throw (RuntimeError("Attempt to increment past the end() AddressInfoIterator"));
		}

		m_pos = m_pos->next();

		return *this;
	}

	const AddressInfo& operator*() {
		if (!m_pos) {
			cosmos_throw (RuntimeError("Attempt to dereference an invalid AddressInfoIterator"));
		}

		return *m_pos;
	}

	bool operator==(const AddressInfoIterator &other) const {
		return m_pos == other.m_pos;
	}

	bool operator!=(const AddressInfoIterator &other) const {
		return !(*this == other);
	}

protected: // data

	const AddressInfo *m_pos = nullptr;
};

} // end ns
