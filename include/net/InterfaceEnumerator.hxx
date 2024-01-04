#pragma once

// cosmos
#include "cosmos/dso_export.h"
#include "cosmos/net/InterfaceInfo.hxx"
#include "cosmos/net/InterfaceIterator.hxx"
#include "cosmos/net/types.hxx"

namespace cosmos {

/// Enumerate all local network interfaces.
/**
 * This type allows to retrieve a list of all local network interfaces. The
 * network interface names and interface indexes are available.
 *
 * To get a new snapshot of the network interface list call fetch(). The
 * begin() and end() iterators allow to iterate over all found entries.
 *
 * Iterating over an empty enumerator instance is allowed and will yield an
 * empty list.
 **/
class COSMOS_API InterfaceEnumerator {
public: // functions
	~InterfaceEnumerator() {
		clear();
	}

	/// Fetch a new snapshot of InterfaceInfo instances.
	void fetch();

	/// Clear a previously fetched result.
	void clear();

	InterfaceIterator begin() {
		return InterfaceIterator{m_list};
	}

	InterfaceIterator end() {
		return InterfaceIterator{};
	}

	bool valid() const {
		return m_list != nullptr;
	}

protected: // data
	InterfaceInfo *m_list = nullptr;
};

} // end ns
