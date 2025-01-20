#pragma once

// Linux
#include <poll.h>

// cosmos
#include "cosmos/BitMask.hxx"

/**
 * @file
 *
 * Wrappers around data structures for I/O related facilities.
 **/

namespace cosmos {

/// Bitmask values used in the poll(2) API to describe I/O status.
/**
 * libcosmos doesn't actively use this API currently, but the bitmask is used
 * in some other contexts as well, like in the SigInfo data structure.
 **/
enum class PollEvent : short {
	INPUT       = POLLIN,    ///< there is data to read.
	PRIORITY    = POLLPRI,   ///< an exceptional condition exists: out-of-band data on TCP socket, PTY master has seen a state change on the slave, a cgroup.events file has been modified.
	OUTPUT      = POLLOUT,   ///< writing is possible.
	READ_HANGUP = POLLRDHUP, ///< stream socket peer closed connection, or shut down the writing half of its connection.
	ERROR       = POLLERR,   ///< an error condition exists (also occurs on the write end of a pipe, when the read end has been closed).
	HANGUP      = POLLHUP,   ///< hang up occurred (outstanding data might still be available)
	INVALID     = POLLNVAL,  ///< invalid request, the file descriptor is not open.
	WRBAND      = POLLWRBAND,///< priority data may be written.
};

/// BitMask of PollEvent flags denoting the I/O status of a file.
using PollEvents = BitMask<PollEvent>;

} // end ns
