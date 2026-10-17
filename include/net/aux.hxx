#pragma once

// cosmos
#include <cosmos/error/UsageError.hxx>
#include <cosmos/net/message_header.hxx>
#include <cosmos/time/time.hxx>

namespace cosmos {

/// Different SOL_SOCKET ancillary message types.
enum class SocketMessage : int {
	/// The packet mark, see SocketOptions::setReceiveMark().
	MARK = SO_MARK,
	/// The receive queue priority, see SocketOptions::setReceivePriority().
	PRIORITY = SO_PRIORITY,
	/// Number of dropped packets in the receive queue, see SocketOptions::setReceiveRxQueueOverflow().
	RX_QUEUE_OVERFLOW = SO_RXQ_OVFL,
	/// Absolute timestamp (uint64_t) when to transmit the payload.
	TXTIME = SCM_TXTIME
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

/// Transmit time control message telling the kernel when to transmit the payload.
/**
 * This message contains a timestamp which is interpreted according to the
 * clock previously configured on the socket via
 * SocketOptions::setTxTimeConfig().
 *
 * For the time specification the kernel expects a uint64_t absolute
 * nanosecond value in the ancillary message. This is contrary to the usual
 * `struct timespec` used with clocks. This means negative timestamp values
 * are not allowed (and also don't make sense for sending something in the
 * future).
 *
 * For convenience this type provides a setter based on the usual TimeSpec
 * type which performs the necessary conversion into an absolute uint64_t
 * timestamp. The caller needs to ensure that the CLOCK used there matches the
 * clock configured at the socket.
 **/
class COSMOS_API TxTimeMessage :
		public AncillaryMessage<OptLevel::SOCKET, SocketMessage> {
public: // functions

	void serialize(SendMessageHeader &hdr) const;

	template <ClockType CLOCK>
	void setTime(const TimeSpec<CLOCK> &ts) {
		if (ts.isNegative()) {
			throw UsageError{"negative TimeSpec not allowed"};
		}
		m_abstime = static_cast<uint64_t>(ts.getSeconds()) * 1000 * 1000 * 1000 + static_cast<uint64_t>(ts.getNanoSeconds());
	}

	void setTime(const uint64_t abstime) {
		m_abstime = abstime;
	}

protected: // data

	uint64_t m_abstime = 0;
};

using MarkMessage = ScalarAuxInputMessage<OptLevel::SOCKET, SocketMessage,
      SocketMessage::MARK, uint32_t>;

using PriorityMessage = ScalarAuxInputMessage<OptLevel::SOCKET, SocketMessage,
      SocketMessage::PRIORITY, uint32_t>;

using RxQueueOverflowMessage = ScalarAuxInputMessage<OptLevel::SOCKET, SocketMessage,
      SocketMessage::RX_QUEUE_OVERFLOW, uint32_t>;

} // end ns
