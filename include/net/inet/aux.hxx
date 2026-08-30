#pragma once

// C++
#include <vector>

// cosmos
#include <cosmos/dso_export.h>
#include <cosmos/net/inet/types.hxx>
#include <cosmos/net/message_header.hxx>
#include <cosmos/net/SocketError.hxx>

/**
 * @file
 *
 * The types in this header support serialization and deserialization of
 * ancillary messages used with SocketFamily::INET{,6}.
 **/

namespace cosmos {

/// Return the IP4Message ancillary message type stored in `msg`, if applicable.
inline std::optional<IP4Message> as_ip4_message(const ReceiveMessageHeader::ControlMessage &msg) {
	if (msg.level() == OptLevel::IP) {
		return IP4Message{msg.raw().cmsg_type};
	}

	return std::nullopt;
}

/// Return the IP6Message ancillary message type stored in `msg`, if applicable.
inline std::optional<IP6Message> as_ip6_message(const ReceiveMessageHeader::ControlMessage &msg) {
	if (msg.level() == OptLevel::IPV6) {
		return IP6Message{msg.raw().cmsg_type};
	}

	return std::nullopt;
}

/// Wrapper for the IPMessage::RECVERR ancillary message.
/**
 * IP based datagram sockets can report extended error messages. To receive
 * them the MessageFlag::ERRQUEUE needs to be passed to
 * Socket::receiveMessage().
 **/
template <SocketFamily FAMILY>
class SocketErrorMessage :
		public AncillaryMessage<
			FamilyTraits<FAMILY>::OPT_LEVEL,
			typename FamilyTraits<FAMILY>::CtrlMsg> {
public: // types

	using SocketError = SocketErrorT<FAMILY>;

public: // functions

	SocketErrorMessage() = default;

	explicit SocketErrorMessage(const ReceiveMessageHeader::ControlMessage &msg) {
		deserialize(msg);
	}

	/// Returns whether `msg` contains a SocketErrorMessage.
	static bool matches(const ReceiveMessageHeader::ControlMessage &msg) {
		if constexpr (FAMILY == SocketFamily::INET) {
			if (const auto type4 = as_ip4_message(msg); type4) {
				return *type4 == IP4Message::RECVERR;
			}
		}

		if constexpr (FAMILY == SocketFamily::INET6) {
			if (const auto type6 = as_ip6_message(msg); type6) {
				return *type6 == IP6Message::RECVERR;
			}
		}

		return false;
	}

	/*
	 * disallow copy/assignment for now due to m_error ptr.
	 */
	SocketErrorMessage(const SocketErrorMessage &) = delete;
	SocketErrorMessage& operator=(const SocketErrorMessage&)= delete;

	void deserialize(const ReceiveMessageHeader::ControlMessage &msg);

	/// Returns the currently deserialized SocketError, if any, otherwise nullptr.
	const SocketError* error() const {
		return m_error;
	}

protected: // functions
	std::vector<uint8_t> m_data;
	SocketError *m_error = nullptr;
};

using IP4SocketErrorMessage = SocketErrorMessage<SocketFamily::INET>;
using IP6SocketErrorMessage = SocketErrorMessage<SocketFamily::INET6>;

extern template class COSMOS_API SocketErrorMessage<SocketFamily::INET>;
extern template class COSMOS_API SocketErrorMessage<SocketFamily::INET6>;

} // end ns
