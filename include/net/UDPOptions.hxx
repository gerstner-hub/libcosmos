#pragma once

// Linux
#include <netinet/udp.h>

// cosmos
#include "cosmos/net/SockOptBase.hxx"

namespace cosmos {

// fwd. decl. for friend declarations below
template <SocketFamily family>
class UDPSocketT;

/// UDP level socket option setter/getter helper.
class COSMOS_API UDPOptions :
		public SockOptBase<OptLevel::UDP> {
	template <SocketFamily family>
	friend class UDPSocketT;
public: // functions

	/// Accumulate output data in kernel until the option is disabled again.
	/**
	 * This can be used to accumulate multiple `send()` calls into a
	 * single datagram. Userspace is responsible for managing this cork
	 * i.e. disabling the option at the appropriate time again.
	 *
	 * pushCork() and popCork() are descriptive wrapper functions that
	 * help with this task.
	 **/
	void setCork(const bool on_off) {
		setBoolOption(OptName{UDP_CORK}, on_off);
	}

	void pushCork() {
		setCork(true);
	}

	void popCork() {
		setCork(false);
	}

	/// Configure segmentation (send) offload on this socket using the given segment_size in bytes.
	/**
	 * This feature allows to pass a single buffer containing multiple UDP
	 * datagrams to the kernel. The kernel will pass the large chunk of
	 * data in one piece as long as possible until it will be split up
	 * into individual datagrams (segments) either in software before
	 * passing it on to the hardware, or even in the hardware itself, it
	 * it supports that.
	 *
	 * The `segment_size` defines the size of each individual segment.
	 * When sending data over the socket then it is considered to contain
	 * a multiple of this segment_size, where the last segment can be
	 * shorter. This size needs to be smaller than the MTU and no more
	 * than 64 segments can be sent in a single `send()` call this way.
	 *
	 * Set this to zero to disable the feature.
	 **/
	void setSendOffload(const uint16_t segment_size) {
		setIntOption(OptName{UDP_SEGMENT}, static_cast<int>(segment_size));
	}

	/// Configure GRO (receive) offload on this socket.
	/**
	 * This is the reverse of setSendOffload(), for the receiving path of
	 * a UDP socket. The socket can then receive multiple datagrams in one
	 * `recv()` system call.
	 *
	 * The segment size of each datagram will be reported via a control
	 * message provided via the `recvmsg()` system call. You need to pass
	 * a four byte control message buffer with `cmsg_type == UDP_GRO` to
	 * receive it.
	 **/
	void setReceiveOffload(const bool on_off) {
		setBoolOption(OptName{UDP_GRO}, on_off);
	}

protected: // functions

	using SockOptBase::SockOptBase;
};

}; // end ns
