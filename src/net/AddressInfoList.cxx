// cosmos
#include <cosmos/error/ResolveError.hxx>
#include <cosmos/net/AddressInfoList.hxx>

namespace cosmos {

void AddressInfoList::resolve(const SysString node, const SysString service) {
	clear();

	const auto res = ::getaddrinfo(
		   node.empty() ? nullptr :    node.raw(),
		service.empty() ? nullptr : service.raw(),
		&m_hints, &m_addrs
	);

	if (res != 0) {
		throw ResolveError{ResolveError::Code{res}};
	}
}

void AddressInfoList::clear() {
	if (m_addrs) {
		::freeaddrinfo(m_addrs);
		m_addrs = nullptr;
	}
}

} // end ns
