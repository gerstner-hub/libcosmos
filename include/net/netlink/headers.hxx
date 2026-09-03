#pragma once

// C++
#include <cstddef>
#include <cstdint>

// cosmos
#include <cosmos/BitMask.hxx>
#include <cosmos/error/errno.hxx>
#include <cosmos/net/netlink/types.hxx>
#include <cosmos/utils.hxx>

/**
 * @file
 *
 * This file contains various standard header types used in the payload
 * application protocol of Netlink sockets.
 **/

namespace cosmos {

/// Common Netlink header which is found in front payload in Netlink packets.
struct COSMOS_API NetlinkHeader :
		protected nlmsghdr {
public: // types

	/// Netlink message type.
	/**
	 * This defines the standard Netlink message types. The Netlink family
	 * application protocol can define additional types like in rtnetlink.
	 **/
	enum class MsgType : uint16_t {
		/// message is to be ignored.
		NOOP  = NLMSG_NOOP,
		/// NetlinkErrorMsg is found in the payload.
		ERROR = NLMSG_ERROR,
		/// A multipart message is terminated with this message.
		DONE  = NLMSG_DONE,
	};

	/// Netlink Message state / informational flags.
	enum class Flag  : uint16_t {
		/* standard flags */

		/// Must be set on all request messages.
		REQUEST = NLM_F_REQUEST,
		/// Message is part of a multipart message terminated by MsgType::DONE.
		MULTI   = NLM_F_MULTI,
		/// Request for an acknowledgment message on success.
		ACK     = NLM_F_ACK,
		/// Request to echo this message.
		ECHO    = NLM_F_ECHO,

		/* GET request flags */

		/// Return the complete table instead of a single entry.
		ROOT    = NLM_F_ROOT,
		/// Return entries matching criteria in payload. Not implemented according no `netlink(7)`.
		MATCH   = NLM_F_MATCH,
		/// Return an atomic snapshot of the table. Requires `CAP_NET_ADMIN` or effective UID 0.
		ATOMIC  = NLM_F_ATOMIC,
		DUMP    = NLM_F_ROOT|NLM_F_MATCH,

		/* NEW request flags */

		/// Replace an existing matching object.
		REPLACE = NLM_F_REPLACE,
		/// Don't replace if the object already exists.
		EXCL    = NLM_F_EXCL,
		/// Create object if it doesn't already exist.
		CREATE  = NLM_F_CREATE,
		/// Add to the end of the object list.
		APPEND  = NLM_F_APPEND,
	};

	using Flags = BitMask<Flag>;

public: // data

	static constexpr size_t HEADER_SIZE = sizeof(struct nlmsghdr);

public: // functions

	/// Creates an empty header of a copy of `hdr`.
	explicit NetlinkHeader(const nlmsghdr *hdr = nullptr);

	/// The size of the message including the header.
	uint32_t length() const {
		return nlmsg_len;
	}

	/// Set the size of the message including the header.
	void setLength(const uint32_t bytes) {
		nlmsg_len = bytes;
	}

	/// The size of the message payload excluding the header data.
	uint32_t payloadLength() const {
		return nlmsg_len > HEADER_SIZE ? nlmsg_len - HEADER_SIZE : 0;
	}

	/// Set the size of the message excluding the header.
	void setPayloadLength(const uint32_t bytes) {
		nlmsg_len = bytes + HEADER_SIZE;
	}

	MsgType type() const {
		return MsgType{nlmsg_type};
	}

	void setType(const MsgType type) {
		nlmsg_type = to_integral(type);
	}

	Flags flags() const {
		return Flags{nlmsg_flags};
	}

	void setFlags(const Flags flags) {
		nlmsg_flags = flags.raw();
	}

	/// Sequence number for the packet.
	uint32_t seqNr() const {
		return nlmsg_seq;
	}

	void setSeqNr(const uint32_t seq) {
		nlmsg_seq = seq;
	}

	NetlinkPort senderID() const {
		return NetlinkPort{nlmsg_pid};
	}

	void setSenderID(const NetlinkPort port) {
		nlmsg_pid = to_integral(port);
	}

	const struct nlmsghdr* raw() const {
		return this;
	}

protected: // functions

	struct nlmsghdr* raw() {
		return this;
	}
};

/// Standard error reply in the Netlink protocol.
/**
 * This message most prominently contains an error code and the header of the
 * message the error code refers to. If NetlinkHeader::Flag::ACK is in effect
 * then a message of this type with error() == Errno::SUCCESS is reported on
 * success.
 *
 * If an actual error occurred and `NetlinkOptions::setEnableCapAcks()` is not
 * in effect then the payload of the original message is appended to the
 * NetlinkErrorMsg. There is currently no accessor for this payload data in
 * this type.
 **/
class NetlinkErrorMsg :
		protected nlmsgerr {
public: // functions

	/// Returns the error code stored in the message.
	Errno error() const {
		return Errno{raw()->error};
	}

	void setError(const Errno err) {
		raw()->error = to_integral(err);
	}

	/// Returns the header of the message that caused the error.
	NetlinkHeader msgHeader() const {
		return NetlinkHeader{&this->msg};
	}

	void setMsgHeader(const NetlinkHeader &hdr);

	struct nlmsgerr* raw() {
		return this;
	}

	const struct nlmsgerr* raw() const {
		return this;
	}
};

} // end ns
