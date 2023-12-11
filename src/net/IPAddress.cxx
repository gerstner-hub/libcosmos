// C++
#include <cstring>

// Cosmos
#include "cosmos/error/ApiError.hxx"
#include "cosmos/error/RuntimeError.hxx"
#include "cosmos/net/IPAddress.hxx"

namespace cosmos {

void* IPAddressBase::ipAddrPtr() {
	return const_cast<void*>(static_cast<const IPAddressBase&>(*this).ipAddrPtr());
}

const void* IPAddressBase::ipAddrPtr() const {
	if (isV4()) {
		return &reinterpret_cast<const sockaddr_in*>(basePtr())->sin_addr;
	} else {
		return &reinterpret_cast<const sockaddr_in6*>(basePtr())->sin6_addr;
	}
}

std::string IPAddressBase::ipAsString() const {
	std::string ret;
	ret.resize(64);

	while (!::inet_ntop(
			to_integral(this->family()),
			ipAddrPtr(),
			ret.data(),
			ret.size())) {
		if (ret.size() > 512) {
			cosmos_throw (ApiError("inet_ntop"));
		}
		ret.resize(ret.size() * 2);
	}

	ret.resize(std::strlen(ret.data()));
	return ret;
}

void IPAddressBase::setIpFromString(const std::string_view sv) {
	const auto res = ::inet_pton(
			to_integral(this->family()),
			sv.data(),
			ipAddrPtr());

	if (res < 0) {
		cosmos_throw (ApiError("inet_pton"));
	} else if (res == 0) {
		cosmos_throw (RuntimeError("inet_pton: bad IP address string"));
	}
}

} // end ns
