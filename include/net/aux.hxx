#pragma once

// cosmos
#include <cosmos/net/message_header.hxx>

namespace cosmos {

enum class SocketMessage : int {
	/// The packet mark, see SocketOptions::setReceiveMark().
	MARK = SO_MARK,
	/// The receive queue priority, see SocketOptions::setReceivePriority().
	PRIORITY = SO_PRIORITY
};

inline std::optional<SocketMessage> as_socket_message(const ReceiveMessageHeader::ControlMessage &msg) {
	if (msg.level() == OptLevel::SOCKET) {
		return SocketMessage{msg.raw().cmsg_type};
	}

	return std::nullopt;
}

/**
 * @file
 *
 * The types in this header support serialization and deserialization of
 * ancillary messages used on generic socket level.
 **/

using MarkMessage = ScalarAuxInputMessage<OptLevel::SOCKET, SocketMessage, SocketMessage::MARK, uint32_t>;

using PriorityMessage = ScalarAuxInputMessage<OptLevel::SOCKET, SocketMessage, SocketMessage::PRIORITY, uint32_t>;

} // end ns
