// cosmos
#include "cosmos/error/ResolveError.hxx"
#include "cosmos/net/AddressInfoList.hxx"

namespace cosmos {

void AddressInfoList::resolve(const std::string_view node, const std::string_view service) {
	clear();

	const auto res = ::getaddrinfo(
		   node.empty() ? nullptr :    node.data(),
		service.empty() ? nullptr : service.data(),
		&m_hints, &m_addrs
	);

	if (res != 0) {
		cosmos_throw(ResolveError(ResolveError::Code{res}));
	}
}

void AddressInfoList::clear() {
	if (m_addrs) {
		::freeaddrinfo(m_addrs);
		m_addrs = nullptr;
	}
}

} // end ns
