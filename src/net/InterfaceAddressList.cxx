// cosmos
#include <cosmos/error/ApiError.hxx>
#include <cosmos/net/InterfaceAddressList.hxx>

namespace cosmos {

void InterfaceAddressList::fetch() {
	clear();
	if (::getifaddrs(&m_addrs) != 0) {
		throw ApiError{"getifaddrs()"};
	}
}

void InterfaceAddressList::clear() {
	if (m_addrs) {
		::freeifaddrs(m_addrs);
		m_addrs = nullptr;
	}
}

} // end ns
