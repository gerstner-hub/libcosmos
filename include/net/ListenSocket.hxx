#ifndef COSMOS_LISTEN_SOCKET_HXX
#define COSMOS_LISTEN_SOCKET_HXX

// cosmos
#include "cosmos/net/Socket.hxx"

namespace cosmos {

/// Base class for connection based listening-only sockets.
/**
 * This base class is only used for connection based socket types. This is
 * further limited to sockets that accept new incoming connections but aren't
 * used for any actual payload I/O (server side sockets).
 **/
class COSMOS_API ListenSocket :
		public Socket {
public: // functions

	using Socket::listen;

protected: // functions

	using Socket::Socket;

protected:

	/*
	 * I/O doesn't make sense on a listen socket
	 */

	using StreamIO::read;
	using StreamIO::readAll;
	using StreamIO::write;
	using StreamIO::writeAll;
};

} // end ns

#endif // inc. guard
