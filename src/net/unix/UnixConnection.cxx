#include <cosmos/net/unix/UnixConnection.hxx>

namespace cosmos {

SocketType UnixConnection::type() const {
	/*
	 * this requires a system call, if we want to avoid that we'd need to
	 * change the constructor of UnixConnection.
	 */
	return this->sockOptions().type();
}

} // end ns
