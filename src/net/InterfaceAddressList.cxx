// cosmos
#include "cosmos/error/ApiError.hxx"
#include "cosmos/net/InterfaceAddressList.hxx"

namespace cosmos {

// this is only an interface wrapper around the system structure, no
// additional data must be added so that pointers can be casted from ifaddrs
// to InterfaceAddress.
static_assert(sizeof(InterfaceAddress) == sizeof(struct ifaddrs));

void InterfaceAddressList::fetch() {
	clear();
	if (::getifaddrs(&m_addrs) != 0) {
		cosmos_throw (ApiError("getifaddrs"));
	}
}

void InterfaceAddressList::clear() {
	if (m_addrs) {
		::freeifaddrs(m_addrs);
		m_addrs = nullptr;
	}
}

} // end ns
