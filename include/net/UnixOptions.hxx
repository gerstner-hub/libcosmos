#ifndef COSMOS_UNIX_OPTIONS_HXX
#define COSMOS_UNIX_OPTIONS_HXX

// C++
#include <chrono>

// cosmos
#include "cosmos/net/SockOptBase.hxx"
#include "cosmos/proc/process.hxx"

namespace cosmos {

// fwd. decl. for friend declarations below
template <SocketFamily family>
class TCPListenSocketT;

/// UnixSocket level option setter/getter helper.
class COSMOS_API UnixOptions :
		// historically UNIX sockets use the base socket option level
		public SockOptBase<OptLevel::SOCKET> {
	friend class UnixDatagramSocket;
	friend class UnixConnection;
	friend class UnixListenSocket;
	friend class UnixClientSocket;
public: // types

	/// User and group credentials of a peer process.
	struct Credentials :
			protected ::ucred {
		auto processID() { return ProcessID{pid}; }
		auto groupID() { return GroupID{gid}; }
		auto userID() { return UserID{uid}; }
	};

public: // functions

	/// This enables or disables the reception of SCM_CREDENTIALS control messages.
	/**
	 * If enabled then this message can be passed from connected-to
	 * processes.
	 *
	 * \see credentials()
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
	Credentials credentials() const;

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

#endif // inc. guard
