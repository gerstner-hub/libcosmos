#pragma once

// C++
#include <vector>

// cosmos
#include <cosmos/net/SockOptBase.hxx>
#include <cosmos/net/unix/aux.hxx>
#include <cosmos/proc/ProcessFile.hxx>

#ifndef SO_RIGHTS_NOTRUNC
	// this was only added in kernel 7.3, not yet found in most userspace
	// headers
#	define SO_RIGHTS_NOTRUNC 85 /* generic UAPI value */
#endif

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

	/// Enable or disable the transfer of UnixMessage::CREDENTIALS ancillary messages.
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

	bool getPassCredentials() const {
		return getBoolOption(OptName{SO_PASSCRED});
	}

	/// Enable or disable the transfer of UnixMessage::RIGHTS ancillary messages.
	/**
	 * This is a rather new addition present in kernel 6.16 onwards. It
	 * allows to _disable_ the generation of UnixMessage::RIGHTS, which is
	 * historically enabled by default on UNIX systems. This is an opt-out
	 * which allows for additional security, since reception of random
	 * file descriptors on the end of a process not prepared to handle
	 * this can cause bad effects (like file descriptor exhaustion).
	 **/
	void setPassRights(const bool on_off) {
		setBoolOption(OptName{SO_PASSRIGHTS}, on_off);
	}

	bool getPassRights() const {
		return getBoolOption(OptName{SO_PASSRIGHTS});
	}

	/// Control the error handling behaviour upon file descriptor reception.
	/**
	 * This option allows to control the behaviour when a Linux security
	 * module (LSM) denies the transfer of a file descriptor from the peer
	 * to this process via a UnixRightsMessage.
	 *
	 * By default the control message is truncated as soon as one such
	 * file descriptor is encountered during processing. With this option
	 * enabled a negative errno is passed instead of the file descriptor,
	 * following file descriptors can still be received.
	 *
	 * This options is only available in kernel 7.3 onwards.
	 **/
	void setRightsNoTruncation(const bool on_off) {
		setBoolOption(OptName{SO_RIGHTS_NOTRUNC}, on_off);
	}

	bool getRightsNoTruncation() const {
		return getBoolOption(OptName{SO_RIGHTS_NOTRUNC});
	}

	/// Enable or disable the reception of UnixMessage::SECURITY ancillary messages.
	/**
	 * This message contains the SELinux security label of the peer socket.
	 **/
	void setPassSecurity(const bool on_off) {
		setBoolOption(OptName{SO_PASSSEC}, on_off);
	}

	void getPassSecurity() const {
		getBoolOption(OptName{SO_PASSSEC});
	}

	/// Enable or disable the reception of UnixMessage::PASSPIDFD ancillary messages.
	/**
	 * This ancillary message transfers a PIDFD file descriptor for the
	 * peer socket into the receiving process. Note that every receive
	 * operation on the socket will provide one such socket until the
	 * option is disabled again. Thus this is mostly useful for
	 * UnixDatagramSocket where multiple peers can talk to the same
	 * socket.
	 *
	 * \see PidFdMessage
	 **/
	void setPassPidFd(const bool on_off) {
		setBoolOption(OptName{SO_PASSPIDFD}, on_off);
	}

	bool getPassPidFD() const {
		return getBoolOption(OptName{SO_PASSPIDFD});
	}

	/// Returns the credentials of the peer process.
	/**
	 * This is used for UnixDomainSockets to identify the credentials of
	 * the peer process. These credentials are stored in the kernel during
	 * `connect()` or `socketpair()` of the related socket.
	 *
	 * The returned UserID and GroupID are the effective UID and GID of
	 * the peer process. The returned information is relating to the time
	 * the peer process called `connect()` on the socket.
	 **/
	UnixCredentials credentials() const;

	/// Returns a vector of supplementary group IDs the peer process owns.
	/**
	 * This is a poorly documented Linux-specific socket option which
	 * allows to retrieve the list of supplementary group IDs the peer
	 * process is associated with.
	 **/
	std::vector<GroupID> supplementaryGroups() const;

	/// Retrieve a ProcessFile representing the peer process.
	/**
	 * The returned PidFD, wrapped in a ProcessFile type, refers to the
	 * peer connected to the UNIX domain socket.
	 *
	 * This a poorly documented Linux-specific socket option available
	 * from kernel 6.9 onwards.
	 *
	 * The returned file will have the O_CLOEXEC flag set.
	 *
	 * This can throw an ApiError with:
	 * - Errno::SEARCH if the peer process no longer exists.
	 * - Errno::NO_DATA if the information is not available.
	 **/
	ProcessFile pidfd() const;

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

	/// Controls the reception of UnixMessage::INQ ancillary messages.
	/**
	 * Use the InQueueMessage type to extract this ancillary message
	 * during reception. This message informs userspace about the
	 * remaining amount of data in the kernel's receive queue for the
	 * socket.
	 *
	 * \see InQueueMessage
	 **/
	void setReceiveInQueue(const bool on_off) {
		setBoolOption(OptName{SO_INQ}, on_off);
	}

	bool getReceiveInQueue() const {
		return getBoolOption(OptName{SO_INQ});
	}

protected: // functions

	using SockOptBase::SockOptBase;
};

}; // end ns
