#pragma once

// C++
#include <vector>

// cosmos
#include <cosmos/dso_export.h>
#include <cosmos/net/ancillary.hxx>
#include <cosmos/net/inet/types.hxx>
#include <cosmos/net/message_header.hxx>
#include <cosmos/net/SocketError.hxx>
#include <cosmos/net/traits.hxx>

/**
 * @file
 *
 * The types in this header support serialization and deserialization of
 * ancillary messages used with SocketFamily::INET{,6}.
 **/

namespace cosmos {

template<>
struct ControlMsgTraits<IP4Message> {
	/* for legacy reasons UnixMessage is based on SOL_SOCKET */
	static constexpr OptLevel LEVEL = OptLevel::IP;
};

template<>
struct ControlMsgTraits<IP6Message> {
	/* for legacy reasons UnixMessage is based on SOL_SOCKET */
	static constexpr OptLevel LEVEL = OptLevel::IPV6;
};

/// Wrapper for the IPMessage::RECVERR ancillary message.
/**
 * IP based datagram sockets can report extended error messages. To receive
 * them the MessageFlag::ERRQUEUE needs to be passed to
 * Socket::receiveMessage().
 **/
template <SocketFamily FAMILY>
class SocketErrorMessage :
		public AncillaryRxMessage<FamilyTraits<FAMILY>::CtrlMsg::RECVERR> {
public: // types

	using SocketError = SocketErrorT<FAMILY>;

public: // functions

	SocketErrorMessage() = default;

	explicit SocketErrorMessage(const ReceiveMessageHeader::ControlMessage &msg) {
		deserialize(msg);
	}

	/*
	 * disallow copy/assignment for now due to m_error ptr.
	 */
	SocketErrorMessage(const SocketErrorMessage &) = delete;
	SocketErrorMessage& operator=(const SocketErrorMessage&)= delete;

	void deserialize(const ReceiveMessageHeader::ControlMessage &msg) override;

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
