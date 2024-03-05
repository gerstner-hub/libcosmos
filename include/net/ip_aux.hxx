#pragma once

// C++
#include <cstring>
#include <vector>

// Cosmos
#include <cosmos/dso_export.h>
#include <cosmos/net/message_header.hxx>
#include <cosmos/net/SocketError.hxx>

namespace cosmos {

/**
 * @file
 *
 * The types in this header support serialization and deserialization of
 * ancillary messages used with SocketFamily::INET{,6}.
 **/

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

	void deserialize(const ReceiveMessageHeader::ControlMessage &msg);

	/// Returns the currently deserialized SocketError, if any, otherwise nullptr.
	const SocketError* error() const {
		if (m_data.empty())
			return nullptr;

		return reinterpret_cast<const SocketError*>(m_data.data());
	}

protected: // functions
	std::vector<uint8_t> m_data;
};

using IP4SocketErrorMessage = SocketErrorMessage<SocketFamily::INET>;
using IP6SocketErrorMessage = SocketErrorMessage<SocketFamily::INET6>;

extern template class COSMOS_API SocketErrorMessage<SocketFamily::INET>;
extern template class COSMOS_API SocketErrorMessage<SocketFamily::INET6>;

} // end ns
