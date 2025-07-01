// cosmos
#include <cosmos/error/RuntimeError.hxx>
#include <cosmos/net/SocketAddress.hxx>
#include <cosmos/utils.hxx>

namespace cosmos {

void SocketAddress::update(size_t new_length) {
	if (new_length != this->size()) {
		throw RuntimeError{"inconsistent socket address size on return"};
	}
}

void SocketAddress::clear() {
	auto addr = basePtr();
	std::memset(addr, 0, maxSize());
	addr->sa_family = to_integral(family());
}

}; // end ns
