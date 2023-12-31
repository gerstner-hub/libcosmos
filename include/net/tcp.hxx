#pragma once

// Linux
#include <netinet/tcp.h>

namespace cosmos {

/// This structure provides detailed information about TCP socket state.
struct TCPInfo :
	public tcp_info {

};

}; // end ns
