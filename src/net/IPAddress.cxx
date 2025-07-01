// C++
#include <cstring>

// Cosmos
#include <cosmos/error/ApiError.hxx>
#include <cosmos/error/ResolveError.hxx>
#include <cosmos/error/RuntimeError.hxx>
#include <cosmos/net/IPAddress.hxx>

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
			throw ApiError{"inet_ntop()"};
		}
		ret.resize(ret.size() * 2);
	}

	ret.resize(std::strlen(ret.data()));
	return ret;
}

void IPAddressBase::setIpFromString(const SysString str) {
	const auto res = ::inet_pton(
			to_integral(this->family()),
			str.raw(),
			ipAddrPtr());

	if (res < 0) {
		throw ApiError{"inet_pton()"};
	} else if (res == 0) {
		throw RuntimeError{"inet_pton: bad IP address string"};
	}
}

void IPAddressBase::getNameInfo(std::string &host, std::string &service, const NameInfoFlags flags) {
	getNameInfo(&host, &service, flags);
}

std::string IPAddressBase::getHostInfo(const NameInfoFlags flags) {
	std::string ret;

	getNameInfo(&ret, nullptr, flags);

	return ret;
}

std::string IPAddressBase::getServiceInfo(const NameInfoFlags flags) {
	std::string ret;

	getNameInfo(nullptr, &ret, flags);

	return ret;
}

void IPAddressBase::getNameInfo(std::string *host, std::string *service, const NameInfoFlags flags) {

	if (host) {
		host->resize(MAX_HOSTNAME);
	}

	if (service) {
		service->resize(MAX_SERVICE);
	}

	const auto res = ::getnameinfo(
			this->basePtr(), this->size(),
			host    ? host->data()    : nullptr, host    ? host->size()    : 0,
			service ? service->data() : nullptr, service ? service->size() : 0,
			flags.raw());

	if (res != 0) {
		throw ResolveError{ResolveError::Code{res}};
	}

	if (host) {
		host->resize(std::strlen(host->c_str()));
	}

	if (service) {
		service->resize(std::strlen(service->c_str()));
	}
}

} // end ns
