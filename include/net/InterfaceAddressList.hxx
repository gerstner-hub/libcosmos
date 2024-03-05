#pragma once

// cosmos
#include <cosmos/dso_export.h>
#include <cosmos/net/InterfaceAddress.hxx>
#include <cosmos/net/InterfaceAddressIterator.hxx>

namespace cosmos {

/// Access to the list of local network interface addresses.
/**
 * This class obtains the current list of local network interface addresses.
 * Obtain a snapshot of the list via fetch() and iterate over the fetched
 * addresses use the begin()/end() iterators.
 *
 * For each network interface multiple addresses can be reported e.g. for
 * IPv4, IPv6 and Packet (ethernet layer, MAC address).
 **/
class COSMOS_API InterfaceAddressList {
public: // functions

	~InterfaceAddressList() {
		clear();
	}

	/// Fetch a snapshot of the current list of network interface addresses.
	void fetch();

	/// Clear stored interface addresses.
	/**
	 * Iterating over a cleared InterfaceAddressList is allowed but yields
	 * no entries.
	 **/
	void clear();

	/// Returns whether currently a list of network interface addresses is available.
	bool valid() const {
		return m_addrs != nullptr;
	}

	InterfaceAddressIterator begin() const {
		return InterfaceAddressIterator{m_addrs};
	}

	InterfaceAddressIterator end() const {
		return InterfaceAddressIterator{nullptr};
	}

protected: // data

	struct ifaddrs *m_addrs = nullptr;
};

} // end ns
