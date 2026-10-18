#pragma once

// Linux
#include <time.h>
#include <linux/errqueue.h>
#include <linux/net_tstamp.h>

// C++
#include <optional>

// cosmos
#include <cosmos/error/UsageError.hxx>
#include <cosmos/net/ancillary.hxx>
#include <cosmos/net/message_header.hxx>
#include <cosmos/net/SocketOptions.hxx>
#include <cosmos/time/time.hxx>

/**
 * @file
 *
 * The types in this header support serialization and deserialization of
 * ancillary messages used on generic socket level.
 **/

namespace cosmos {

/// Different SOL_SOCKET ancillary message types.
enum class SocketMessage : int {
	/// The packet mark, see SocketOptions::setReceiveMark().
	MARK                 = SO_MARK,
	/// The receive queue priority, see SocketOptions::setReceivePriority().
	PRIORITY             = SO_PRIORITY,
	/// Number of dropped packets in the receive queue, see SocketOptions::setReceiveRxQueueOverflow().
	RX_QUEUE_OVERFLOW    = SO_RXQ_OVFL,
	/// Absolute timestamp (uint64_t) when to transmit the payload.
	TXTIME               = SCM_TXTIME,
	/// `struct so_timestamping` containing three different timestamps.
	TIMESTAMPING         = SCM_TIMESTAMPING,
	/// `struct scm_ts_pktinfo` containing additional layer2 info.
	TIMESTAMPING_PKTINFO = SCM_TIMESTAMPING_PKTINFO,
	/// `uint32` explicit packet ID value; send only.
	TS_OPT_ID            = SCM_TS_OPT_ID,
};

template<>
struct ControlMsgTraits<SocketMessage> {
	static constexpr OptLevel LEVEL = OptLevel::SOCKET;
};

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
		public AncillaryTxMessage<SocketMessage::TXTIME> {
public: // functions

	void serialize(SendMessageHeader &hdr) const override;

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

/// Parse SO_TIMESTAMPING ancillary messages.
/**
 * This data structure contains three timestamps, one of which is deprecated
 * by now. The remaining two are software-generated vs. hardware-generated
 * timestamps. Depending on NIC, device driver support and SO_TIMESTAMPING
 * configuration only one of both may be supplied. As a result this type only
 * returns optional timestamp values.
 *
 * The software timestamp is always based on the system's CLOCK_REALTIME,
 * while the hardware timestamp is usually based on a device and/or
 * system-specific PTP clock. For this reason this type returns a special
 * UnknownTime type, given a lack of means to properly interpret the
 * timestamp at the moment.
 *
 * For RX timestamping the timestamps are always related to the payload which
 * is received along with the ancillary message.
 *
 * For TX timestamping a call to Socket::recvMessage() with
 * MessageFlag::ERRQUEUE needs to be made. The timestamps will then be
 * received together with a SocketErrorMessage of Origin::TXSTATUS containing
 * an errno of Errno::NO_MESSAGE. The original payload will be received back
 * as payload data, unless SocketOptions::TimestampingConfig::OPT_TSONLY is
 * set.
 *
 * \see SocketOptions::setReceiveTimestamping().
 **/
class COSMOS_API TimestampingMessage :
		public AncillaryRxMessage<SocketMessage::TIMESTAMPING> {

protected: // functions

	const auto& hwStamp() const {
		return m_ts.ts[2];
	}

	const auto& swStamp() const {
		return m_ts.ts[0];
	}

	bool isZero(auto ts) const {
		return ts.tv_sec != 0 || ts.tv_nsec != 0;
	}

public: // functions

	void deserialize(const ReceiveMessageHeader::ControlMessage &msg) override;

	bool hasSoftwareStamp() const {
		return !isZero(swStamp());
	}

	bool hasHardwareStamp() const {
		return !isZero(hwStamp());
	}

	std::optional<RealTime> softwareStamp() const {
		const auto sw_stamp = swStamp();
		if (isZero(sw_stamp))
			return {};

		// tv_nsec is long long in kernel_timespec, but on 32-bit in
		// struct timespec this is only 32-bits. shouldn't matter.
		return RealTime{sw_stamp.tv_sec, static_cast<long>(sw_stamp.tv_nsec)};
	}

	std::optional<UnknownTime> hardwareStamp() const {
		const auto hw_stamp = hwStamp();
		if (isZero(hw_stamp))
			return {};

		return UnknownTime{hw_stamp.tv_sec, static_cast<long>(hw_stamp.tv_nsec)};
	}

protected: // data

	struct scm_timestamping64 m_ts{};
};

/// Configure a custom starting point for timestamping packet IDs.
/**
 * This ancillary message can be sent when
 * SocketOption::setReceiveTimestamping() is in effect with the
 * SocketOption::TimestampingConfig::OPT_ID flag set. It will explicitly
 * configure a starting point for packet IDs reported via the socket's error
 * queue.
 **/
class COSMOS_API TimestampingOptIdMessage :
		public AncillaryTxMessage<SocketMessage::TS_OPT_ID> {
public: // functions

	void serialize(SendMessageHeader &hdr) const override;

	void setID(const TimestampPktId id) {
		m_id = id;
	}

protected: // data

	TimestampPktId m_id{};
};

/// Configure TX timestamping for a single send operation.
/**
 * This is a send-only ancillary message related to
 * SocketOptions::setReceiveTimestamping(). The message allows to configure TX
 * timestamping just for the send operation is supplied in. Only the flags
 * `Flag::TX_*` may be specified in this message. Timestamping must be
 * generally enabled via the socket option already for this to work..
 **/
class COSMOS_API TimestampingTxConfigMessage :
		public AncillaryTxMessage<SocketMessage::TIMESTAMPING> {
public: // types

	using Flags = SocketOptions::TimestampingConfig::Flags;
	using enum SocketOptions::TimestampingConfig::Flag;

public: // functions

	void serialize(SendMessageHeader &hdr) const override;

	void setFlags(const Flags flags) {
		m_flags = flags;
	}

protected: // data

	Flags m_flags;
};

/// Additional hardware information about receive timestamps.
/**
 * This message type is related to SocketOption::setReceiveTimestamping() with
 * SocketOption::TimestampingConfig::OPT_PKTINFO enabled. The message is
 * provided alongside TimestampingMessage if it contains a hardware timestamp
 * and if the NIC driver is using NAPI. This also requires kernel option
 * CONFIG_NET_RX_BUSY_POLL to be enabled.
 **/
class COSMOS_API TimestampingPacketInfo :
		public AncillaryRxMessage<SocketMessage::TIMESTAMPING_PKTINFO > {
public: // types

	struct PacketInfo :
			protected scm_ts_pktinfo {
	public:

		/// The index of the real device which received the packet.
		InterfaceIndex index() const {
			/* this uses a uint32 instead of int */
			return InterfaceIndex{(int)this->if_index};
		}

		/// Returns the layer2 packet length.
		uint32_t packetLength() const {
			return this->pkt_length;
		}

		scm_ts_pktinfo* raw() {
			return this;
		}

		const scm_ts_pktinfo* raw() const {
			return this;
		}
	};

public: // functions

	void deserialize(const ReceiveMessageHeader::ControlMessage &msg) override;

	/// Access the currently deserialized data.
	/**
	 * This only contains valid data after deserialize() was called at
	 * least once.
	 **/
	const PacketInfo& info() const {
		return m_info;
	}

protected:

	PacketInfo m_info{};
};

using MarkMessage = ScalarAuxRxMessage<uint32_t, SocketMessage::MARK>;

using PriorityMessage = ScalarAuxRxMessage<uint32_t, SocketMessage::PRIORITY>;

using RxQueueOverflowMessage = ScalarAuxRxMessage<uint32_t, SocketMessage::RX_QUEUE_OVERFLOW>;

} // end ns
