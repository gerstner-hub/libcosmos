#pragma once

// C++
#include <cstring>
#include <optional>

// Cosmos
#include <cosmos/dso_export.h>
#include <cosmos/error/RuntimeError.hxx>
#include <cosmos/io/iovector.hxx>
#include <cosmos/net/SocketAddress.hxx>
#include <cosmos/net/types.hxx>

/**
 * @file
 *
 * Special types and wrappers used in the Socket::sendMessage() and
 * Socket::receiveMessage() APIs.
 *
 * These APIs are pretty complex as they allow a lot of different system call
 * configurations and also control messages to be passed. The `struct msghdr`
 * behaves quite differently when used for sending as compared to when used
 * for receiving. For this reason we provide different wrappers for both
 * purposes.
 **/

namespace cosmos {

/// Remodelling of `struct msghdr` with const semantics.
/**
 * As this struct takes pointers to payload and control data, these pointers
 * are subject to const semantics issues when sending data. We don't want to
 * const_cast pointers back and forth. For this reason remodel the system data
 * structure as a const variant for sending.
 **/
struct msghdr_const {
	const void *msg_name;
	socklen_t msg_namelen;
	const struct iovec *msg_iov;
	size_t msg_iovlen;
	const void *msg_control;
	size_t msg_controllen;
	int msg_flags;
};

/// Base class for SendMessageHeader and ReceiveMessageHeader.
/**
 * Since the `struct msghdr` has quite different uses for sending vs.
 * receiving, we split up the libcosmos wrappers into two different types.
 * This base class shares common semantics between the two.
 *
 * MSGHDR is the actual `struct msghdr` to be used, since we remodelled
 * `msghdr` as `msghdr_const` for the SendMessageHeader case.
 **/
template <typename MSGHDR>
class MessageHeaderBase {
public: // functions

	/// Create a MSGHDR initialized to all zeroes and with default flags applied.
	MessageHeaderBase() {
		clear();
		// by default mark file descriptor received via unix domain
		// sockets CLOEXEC.
		setIOFlags(MessageFlags{MessageFlag::CLOEXEC});
	}

	/// Clear the complete system call structure with zeroes.
	void clear() {
		std::memset(&m_header, 0, sizeof(m_header));
	}

	/// Set the flags used for sending or receiving data.
	/**
	 * This corresponds to the `flags` argument in `sendmsg()` and
	 * `recvmsg()`. The `msg_flags` field in `msghdr` is actually not used
	 * as an input parameter in these system calls, only as an output
	 * parameter in `recvmsg()`.
	 *
	 * We keep these flags as an extension to `struct msghdr` in
	 * MessageHeaderBase to avoid having to add additional parameters to
	 * Socket::sendMessage() and Socket::receiveMessage().
	 **/
	void setIOFlags(const MessageFlags flags) {
		m_io_flags = flags;
	}

protected: // functions

	/// Reset the address portion of the msghdr struct.
	void resetAddress() {
		m_header.msg_name = nullptr;
		m_header.msg_namelen = 0;
	}

	/// Set the `msg_iov` fields of the msghdr struct based on the given iovector object.
	template <typename IOVEC>
	void setIov(IOVEC &iovec) {
		if (iovec.empty()) {
			m_header.msg_iov = nullptr;
			m_header.msg_iovlen = 0;
		} else {
			m_header.msg_iov = iovec.raw();
			m_header.msg_iovlen = iovec.size();
		}
	}

	/// Returns the currently set MessageFlags for send/receive.
	MessageFlags ioFlags() const {
		return m_io_flags;
	}

protected: // data

	/// The low level `struct msghdr`
	MSGHDR m_header;
	/// The currently configured send/receive flags.
	MessageFlags m_io_flags;
};

/// Wrapper for `struct msghdr` for sending messages via Socket::sendMessage().
/**
 * This type holds extended data for sending a message over a socket. For one
 * it allows sending data from multiple scattered memory regions using a
 * WriteIOVector. Furthermore additional ancillary data can be sent. Both of
 * these items can be set using the public members `iovec` and `control_msg`.
 * These variables will be applied when passing the SendMessageHeader to
 * Socket::sendMessage() or one of its specializations.
 *
 * Libcosmos currently only supports sending a single control message at once.
 * The ControlMessage type can only be constructed by special types that know
 * how to serialize one like the UnixRightsMessage type for sending file
 * descriptors over a UNIX domain socket.
 *
 * There are some restrictions when sending ancillary data. With
 * SocketType::STREAM sockets ancillary data must always be accompanied by
 * some payload data. If no payload data is otherwise available then a dummy
 * byte needs to be sent to make it possible to send ancillary data. On
 * SocketType::DGRAM sockets on Linux it is also possible to send ancillary
 * data without any payload.
 **/
class SendMessageHeader :
		public MessageHeaderBase<msghdr_const> {
	friend class Socket;
public: // types

	/// Wrapper for `struct cmsghdr` used for creating new control messages for sending.
	/**
	 * Only specialized serialization helpers may create instances of this
	 * type. These types know how to serialize their state into a
	 * ControlMessage for sending.
	 **/
	class ControlMessage {
		template <OptLevel, typename MSG_TYPE>
		friend class AncillaryMessage;
		friend class SendMessageHeader;
	protected: // functions

		/// Creates a new control message for the given level, type and size.
		/**
		 * \param[in] type The plain integer denoting the type of
		 * control message. Since this type depends on the OptLevel,
		 * there is no way to use a single strong type here.
		 * Serialization helpers need to ensure that the type and its
		 * value are sane.
		 *
		 * \param[in] data_len The number of bytes that need to be
		 * stored in the control message. This size needs to be known
		 * in advance and cannot be changed during the lifetime of an
		 * object..
		 **/
		ControlMessage(const OptLevel level, int type, const size_t data_len);

		/// Returns the data portion of the control message.
		/**
		 * This is the location where the actual message data needs to
		 * go.
		 **/
		uint8_t* data() {
			return CMSG_DATA(m_header);
		}

		const uint8_t* data() const {
			return CMSG_DATA(m_header);
		}

		/// Returns the amount of bytes that can be stored at data().
		size_t dataSpace() const {
			return data() - m_buffer.data();
		}

		/// Returns the pointer to the complete control message for the `msg_control` field in `struct msghdr`.
		const void* raw() const {
			return m_buffer.data();
		}

		/// Returns the size of the complete control message for the `msg_controllen` field in `struct msghdr`.
		size_t size() const {
			return m_buffer.size();
		}

	protected: // data

		/// The raw data the control message is composed of.
		std::vector<uint8_t> m_buffer;
		/// Pointer to the beginning of m_buffer for setting header data.
		struct cmsghdr *m_header = nullptr;
	};

public: // data

	/// Memory regions to send.
	WriteIOVector iovec;
	/// Control message to send, if any.
	std::optional<ControlMessage> control_msg;

protected: // functions

	/// Prepare a `sendmsg()` operation using the given optional target address.
	void prepareSend(const SocketAddress *addr);

	/// Perform any cleanup or bookkeeping after a successful `sendmsg()` operation.
	void postSend(size_t sent) {
		iovec.update(sent);
		control_msg.reset();
	}

	/// Fill in the target address fields of the `struct msghdr` for the given address object.
	void setAddress(const SocketAddress &addr);

	/// Return a pointer to the raw `struct msghdr` for passing to the `sendmsg()` system call.
	const struct msghdr* rawHeader() const {
		return reinterpret_cast<const struct msghdr*>(&m_header);
	}
};

/// Wrapper for `struct msghdr` for receiving message via Socket::receiveMessage().
/**
 * This type holds extended data for receiving a message over a socket. For
 * one it allows receiving data into multiple scattered memory regions using a
 * ReadIOVector. Furthermore additional ancillary data can be received, if
 * setup via `setControlBufferSize()`. The public `iovec` member is used for
 * setting up the according memory regions for receiving. These settings will
 * be applied when passing the ReceiveMessageHeader to
 * Socket::receiveMessage() or one of its specializations.
 *
 * This type implements an iterator interface to iterate over any received
 * ancillary messages. Beware that ancillary data may arrive in a different
 * order and payload/ancillary data combination than it was sent.
 **/
class COSMOS_API ReceiveMessageHeader :
		public MessageHeaderBase<msghdr> {
	friend class Socket;
public: // types

	/// Wrapper for `struct cmsghdr` used for iterating over received control messages.
	class ControlMessage {
		template <OptLevel, typename MSG_TYPE>
		friend class AncillaryMessage;
	protected: // functions

		/// Returns the raw control message type (which is a different type depending on `level()`.
		int type() const {
			return m_header.cmsg_type;
		}

		/// Returns the length of this control message including the header.
		auto length() const {
			return m_header.cmsg_len;
		}

	public: // functions

		/// This defines the basic option level this control message is for.
		/**
		 * The option level determines how the rest of the control
		 * message is to be interpreted.
		 **/
		OptLevel level() const {
			return OptLevel{m_header.cmsg_level};
		}

		/// Return the UnixMessage ancillary message type, if applicable.
		std::optional<UnixMessage> asUnixMessage() const {
			if (level() == OptLevel::SOCKET) {
				return UnixMessage{type()};
			}

			return std::nullopt;
		}

		std::optional<IP4Message> asIP4Message() const {
			if (level() == OptLevel::IP) {
				return IP4Message{type()};
			}

			return std::nullopt;
		}

		std::optional<IP6Message> asIP6Message() const {
			if (level() == OptLevel::IPV6) {
				return IP6Message{type()};
			}

			return std::nullopt;
		}

		/// Returns the data portion of the control message.
		/**
		 * This pointer is not necessarily suitably aligned to access
		 * arbitrary (casted) data structures through it. Applications
		 * need to copy the data via `memcpy()` into suitably located
		 * data structures.
		 **/
		const void* data() const {
			return CMSG_DATA(&m_header);
		}

		/// The amount of bytes found at data().
		size_t dataLength() const {
			return length() - sizeof(m_header);
		}

	protected: // data

		/// The raw control message header of this ancillary message.
		struct cmsghdr m_header;
	};

	/// Helper type for iterating over ControlMessage instances received in a ReceiveMessageHeader.
	class ControlMessageIterator {
	public: // functions

		/// Create an invalid (end) iterator.
		ControlMessageIterator() {}

		/// Create an iterator pointing to the first ControlMessage of header.
		/**
		 * If there is no ControlMessage in `header` then an invalid
		 * (end) iterator is the result.
		 **/
		explicit ControlMessageIterator(const ReceiveMessageHeader &header) :
				m_pos{CMSG_FIRSTHDR(&header.m_header)},
				m_header{&header} {
		}

		/// Advance to the next ControlMessage, or to the end of the range.
		auto& operator++() {
			if (m_pos) {
				const msghdr *hdr = m_header->rawHeader();
				const cmsghdr *chdr = m_pos;
				m_pos = CMSG_NXTHDR(const_cast<msghdr*>(hdr), const_cast<cmsghdr*>(chdr));
			} else {
				throw RuntimeError{"Attempt to increment ControlMessageIterator past the end"};
			}

			return *this;
		}

		bool operator==(const ControlMessageIterator &other) const {
			return m_pos == other.m_pos;
		}

		bool operator!=(const ControlMessageIterator &other) const {
			return !(*this == other);
		}

		/// Access the current ControlMessage the iterator points to.
		const ControlMessage& operator*() {
			if (!m_pos) {
				throw RuntimeError{"Attempt to dereference an invalid ControlMessageIterator"};
			}

			return *reinterpret_cast<const ControlMessage*>(m_pos);
		}

	protected: // data

		const cmsghdr *m_pos = nullptr;
		const ReceiveMessageHeader *m_header = nullptr;
	};


public: // data

	/// Memory regions to receive data into.
	ReadIOVector iovec;

public: // functions

	/// Returns the MessageFlags provided by the last `recvmsg()` operation.
	MessageFlags flags() const {
		return MessageFlags{m_header.msg_flags};
	}

	/// Set the size of the buffer used for receiving ancillary messages.
	/**
	 * By default no ancillary messages will be received. Setting this
	 * buffer size expresses the intent to receive ControlMessages in
	 * future calls to `Socket::receiveMessage()` or one of its
	 * specializations. This setting remains active until
	 * clearControlBuffer() is called.
	 *
	 * Note that receiving ancillary messages should not be taken lightly,
	 * especially with UNIX domain sockets. Other processes connected to
	 * this socket can now send file descriptors to this process, that
	 * will use up entries in the file descriptor table unless properly
	 * dealt with by the application.
	 **/
	void setControlBufferSize(const size_t bytes);

	/// No longer receive control messages.
	void clearControlBuffer() {
		setControlBufferSize(0);
	}

	ControlMessageIterator begin() {
		return ControlMessageIterator{*this};
	}

	ControlMessageIterator end() {
		return ControlMessageIterator{};
	}

protected: // functions

	/// Prepare a `recvmsg()` operation using the given optional source address storage.
	void prepareReceive(SocketAddress *addr);

	/// Perform any cleanup or bookkeeping after a successful `recvmsg()` operation.
	void postReceive(size_t received) {
		iovec.update(received);
	}

	/// Fill in the source address storage fields of the `struct msghdr` for the given address object.
	void setAddress(SocketAddress &addr);

	struct msghdr* rawHeader() {
		return &m_header;
	}

	const struct msghdr* rawHeader() const {
		return &m_header;
	}

protected: // data

	/// Optional buffer used to receive ancillary messages.
	std::vector<uint8_t> m_control_buffer;
};

/// Base class for types that deal with (de)serializing ancillary socket messages.
/**
 * This base class keeps some common logic that is shared between ancillary
 * socket message types. More importantly this type is here to allow access to
 * the API of SendMessageHeader::ControlMessage without adding a lot of friend
 * declarations for each type of ancillary message.
 **/
template <OptLevel level, typename MSG_TYPE>
class AncillaryMessage {
protected: // functions

	SendMessageHeader::ControlMessage createMsg(MSG_TYPE type, const size_t data_len) const {
		return SendMessageHeader::ControlMessage{level, to_integral(type), data_len};
	}

	void checkMsg(const ReceiveMessageHeader::ControlMessage &msg, MSG_TYPE type) const {
		if (msg.level() != level || type != MSG_TYPE(msg.type())) {
			throw RuntimeError{"ancillary message type mismatch"};
		}
	}

	uint8_t* data(SendMessageHeader::ControlMessage &msg) const {
		return msg.data();
	}
};

} // end ns
