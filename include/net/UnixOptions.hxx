#pragma once

// C++
#include <chrono>

// cosmos
#include "cosmos/net/SockOptBase.hxx"
#include "cosmos/net/unix_aux.hxx"

namespace cosmos {

/// UnixSocket level option setter/getter helper.
class COSMOS_API UnixOptions :
		// historically UNIX sockets use the base socket option level
		public SockOptBase<OptLevel::SOCKET> {
	friend class UnixDatagramSocket;
	friend class UnixConnection;
	friend class UnixListenSocket;
	friend class UnixClientSocket;
public: // functions

	/// This enables or disables the transfer of SCM_CREDENTIALS control messages.
	/**
	 * If enabled then this message can be passed between processes that
	 * communicate via a UNIX domain socket. Note that both sides of the
	 * socket, the sender and the receiver need to enable this to work
	 * properly. Otherwise the message can be seen on the receiver side
	 * but with overflow values filled in for user and group ID and a
	 * ProcessID of 0.
	 *
	 * Note that the ancillary message is not only provided to the
	 * receiving side if the sender explicitly sends the ancillary
	 * message, but also implicitly with each received message. The kernel
	 * fills in default values for the peer process (its PID and real user
	 * and group ID).
	 *
	 * \see credentials()
	 * \see UnixCredentialsMessage
	 **/
	void setPassCredentials(const bool on_off) {
		setBoolOption(OptName{SO_PASSCRED}, on_off);
	}

	/// This enables or disables the reception of SCM_SECURITY ancillary messages.
	/**
	 * This message contains the SELinux security label of the peer socket.
	 **/
	void setPassSecurity(const bool on_off) {
		setBoolOption(OptName{SO_PASSSEC}, on_off);
	}

	/// Returns the credentials of the peer process.
	/**
	 * This is used for UnixDomainSockets to identify the credentials of
	 * the peer process. These credentials are stored in the kernel during
	 * `connect()` or `socketpair()` of the related socket.
	 **/
	UnixCredentials credentials() const;

	/// Sets an offset for the MessageFlag::PEEK receive() flag.
	/**
	 * If enabled then the `recv()` system call combined with
	 * MessageFlag::PEEK will cause data to be returned that is found at the
	 * given byte offset, instead of the beginning of the receive queue.
	 *
	 * If data is removed from the input queue by doing a receive()
	 * without MessageFlag::PEEK then the offset will be decreased by the
	 * removed number of bytes, so that the offset is always pointing to
	 * the same relative position of the input queue.
	 **/
	void setPeekOffset(const bool on_off, const size_t offset = 0) {
		const int off = on_off ? offset : -1;
		setIntOption(OptName{SO_PEEK_OFF}, off);
	}

	using SockOptBase::getPeerSec;

protected: // functions

	using SockOptBase::SockOptBase;
};

}; // end ns
