#ifndef COSMOS_TCP_HXX
#define COSMOS_TCP_HXX

// Linux
#include <netinet/tcp.h>

namespace cosmos {

/// This structure provides detailed information about TCP socket state.
struct TCPInfo :
	public tcp_info {

};

}; // end ns

#endif // inc. guard
