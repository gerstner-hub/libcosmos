#pragma once

// C++
#include <vector>

// Cosmos
#include "cosmos/dso_export.h"
#include "cosmos/fs/types.hxx"
#include "cosmos/net/message_header.hxx"
#include "cosmos/net/types.hxx"
#include "cosmos/proc/process.hxx"

namespace cosmos {

/**
 * @file
 *
 * The types in this header support serialization and deserialization of
 * ancillary messages used with SocketFamily::UNIX.
 **/

/// User and group credentials of a peer process.
/**
 * This type is used to indicate process credentials passed over a UNIX domain
 * socket. It is used in UnixOptions::credentials() and in the
 * UnixCredentialsMessage ancillary message.
 **/
struct COSMOS_API UnixCredentials :
		protected ::ucred {

	/// Create credentials all set to INVALID values.
	UnixCredentials() :
		UnixCredentials{ProcessID::INVALID, UserID::INVALID, GroupID::INVALID} {
	}

	/// Create credentials using the given values.
	UnixCredentials(const ProcessID p_pid, const UserID p_uid, const GroupID p_gid) {
		pid = cosmos::to_integral(p_pid);
		uid = cosmos::to_integral(p_uid);
		gid = cosmos::to_integral(p_gid);
	}

	/// Fill in the credentials from the current process context.
	/**
	 * The current process ID, effective UID and effective GID will be set
	 * in the structure.
	 **/
	void setCurrentCreds();

	auto processID() { return ProcessID{pid}; }
	auto userID() { return UserID{uid}; }
	auto groupID() { return GroupID{gid}; }
};

/// Wrapper for the SCM_RIGHTS socket ancillary message to pass file descriptors to other processes.
/**
 * UNIX domain sockets can be used to pass file descriptors between unrelated
 * processes. This class supports both, assembling an ancillary message to
 * pass on file descriptors to another process, and deserializing an ancillary
 * message to access file descriptors received from another process.
 *
 * For sending add the desired file descriptors to the object using `addFD()`.
 * The object will not take ownership of the file descriptors and will never
 * close them. The file descriptors need to stay valid until the ancillary
 * message has been successfully sent out, though. The final ancillary message
 * can be created using the serialize() method. The resulting
 * SendMessageHeader::ControlMessage can be assigned to the `control_msg`
 * member of a `SendMessageHeader` instance, for sending it via
 * `Socket::sendMessage()` or one of its specializations.
 *
 * For receiving setup a `ReceiveMessageHeader` for use with a UNIX domain
 * socket, call setControlBufferSize() on it to allow reception of ancillary
 * data. On successful reception check for a ControlMessage on
 * OptLevel::SOCKEt and of type UnixMessage::RIGHTS. Once this message
 * arrives, pass it to the `deserialize()` function to parse the file
 * descriptor numbers that have been received. At this point the file
 * descriptors will be allocated in the receiving process and ownership of
 * them needs to be managed. The `takeFDs()` function transfers the ownership
 * of received file descriptors to the caller. This operation can only happen
 * once. If for some reason the file descriptors are never claimed, then they
 * are closed internally upon destruction of the object or before the object
 * state is modified in other ways.
 *
 * There are a number of pitfalls with this mechanism:
 *
 * - when sending a UnixRightsMessage it is best to send some actual payload
 *   in the SendMessageHeader used for this. On Linux when using a
 *   UnixDatagramSocket then this is not strictly necessary. For all other
 *   socket types at least on byte of payload data is necessary for
 *   successfully passing the ancillary message though.
 * - when receiving a UnixRightsMessage then the received file descriptors
 *   will automatically be allocated in the current process. If an application
 *   fails to parse the message or take ownership of the file descriptors then
 *   they will leak. This can lead to a denial-of-service situation especially
 *   if the process at the other end is from a different security domain.
 * - when the control message buffer is too small upon reception of a
 *   UnixRightsMessage then the control message can be truncated (check
 *   Messageflag::CTL_WAS_TRUNCATED). In this case parts of the received file
 *   descriptors will be closed again (or not allocated in the first place).
 * - the order and payload/ancillary message combination used for sending the
 *   file descriptors can change on the receiving side. Design your
 *   application to accept ancillary messages on the receiving end for as long
 *   as you expect such a transmission. Don't wait for a specific payload
 *   message accompanied by the file descriptors.
 **/
class COSMOS_API UnixRightsMessage :
		public AncillaryMessage<OptLevel::SOCKET, UnixMessage> {
public: // types

	/// A vector to keep a series of FileNum file descriptor numbers to pass between processes.
	using FileNumVector = std::vector<FileNum>;

public: // data

	/// Maximum number of file descriptors that can be transferred using a single UnixRightsMessage.
	constexpr static size_t MAX_FDS = 253; /* SCM_MAX_FD, only found in kernel headers */

public: // functions

	~UnixRightsMessage() {
		closeUnclaimed();
	}

	/// Parse received file descriptors from the given ControlMessage.
	/**
	 * If \c msg is not of the right type then an exception is thrown.
	 *
	 * On success check `numFDs()` to learn of the amount of received file
	 * descriptors and use `takeFDs()` to transfer ownership of them to
	 * the caller.
	 **/
	void deserialize(const ReceiveMessageHeader::ControlMessage &msg);

	/// Serialize a ControlMessage for passing file descriptors.
	/**
	 * This will serialize a ControlMessage containing all file
	 * descriptors previously added via `addFD()`.
	 **/
	SendMessageHeader::ControlMessage serialize() const;

	void addFD(cosmos::FileNum fd) {
		m_fds.push_back(fd);
	}

	void clearFDs() {
		closeUnclaimed();
		m_fds.clear();
	}

	void takeFDs(FileNumVector &fds) {
		if (!m_unclaimed_fds) {
			fds.clear();
			return;
		}
		fds = std::move(m_fds);
		m_fds = FileNumVector{};
		m_unclaimed_fds = false;
	}

	size_t numFDs() const {
		return m_unclaimed_fds ? m_fds.size() : 0;
	}

protected: // functions

	void closeUnclaimed();

protected: // data

	FileNumVector m_fds;
	bool m_unclaimed_fds = false; ///< Flag whether "live" FDs in m_fds have not yet been collected.
};

/// Wrapper for the SCM_CREDENTIALS socket ancillary message to transfer process credentials between processes.
/**
 *  This ancillary message carries a UnixCredentials structure consisting of a
 *  ProcessID, UserID and GroupID. Both ends of a UNIX domain socket need to
 *  enable UnixOptions::setPassCredentials() for this to work.
 *
 *  On the sending side only the caller's ProcessID and one of its real,
 *  effective or saved UserIDs and/or GroupIDs may be specified. Privileged
 *  processes with CAP_SYS_ADMIN may specify arbitrary ProcessID, with
 *  CAP_SETUID may specify arbitrary UserIDs and GroupIDs.
 *
 *  Once the socket option is set each received message will carry an implicit
 *  UnixCredentialsMessage, even if the peer did not send one. This is
 *  automatically filled in by the kernel with the process's PID and real
 *  UserID and GroupID.
 **/
class COSMOS_API UnixCredentialsMessage :
		public AncillaryMessage<OptLevel::SOCKET, UnixMessage> {
public: // functions

	void deserialize(const ReceiveMessageHeader::ControlMessage &msg);

	SendMessageHeader::ControlMessage serialize() const;

	void setCreds(const UnixCredentials &creds) {
		m_creds = creds;
	}

	const UnixCredentials& creds() const {
		return m_creds;
	}

protected: // data

	UnixCredentials m_creds;
};

} // end ns
