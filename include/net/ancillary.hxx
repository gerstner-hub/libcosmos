#pragma once

// C++
#include <optional>
#include <cstring>

// cosmos
#include <net/message_header.hxx>
#include <net/traits.hxx>

/**
 * @file
 *
 * Base classes for implementation of ancillary message
 * serialization/deserialization types.
 **/

namespace cosmos {

/// Base class for receive path control messages.
/**
 * This base class supports deserialization of control messages of
 * the given message type TYPE e.g. SocketMessage::MARK.
 **/
template <auto TYPE>
class AncillaryRxMessage {
public: // types

	/// The type of the message type constant like SocketMessage.
	using MSG_TYPE = decltype(TYPE);
	static constexpr OptLevel LEVEL = ControlMsgTraits<MSG_TYPE>::LEVEL;

public: // functions

	/// Checks for a matching message type.
	/**
	 * This helper returns whether `msg` can be handled by the
	 * specialization of this type.
	 **/
	static bool matches(const ReceiveMessageHeader::ControlMessage &msg) {
		if (msg.level() != LEVEL) {
			return false;
		}

		const MSG_TYPE type{msg.raw().cmsg_type};

		return type == TYPE;
	}

	/// Deserialize the given control message.
	/**
	 * This function must be implemented by derived types to interpret the
	 * actual control message payload found in `msg`.
	 *
	 * The implementation should always call `checkMsg(msg)` to verify the
	 * control message is of the proper type first. A RuntimeError is
	 * thrown in case the type does not match.
	 **/
	virtual void deserialize(
			const ReceiveMessageHeader::ControlMessage &msg) = 0;

protected: // functions

	void checkMsg(
			const ReceiveMessageHeader::ControlMessage &msg,
			std::optional<size_t> expected_size = {}) const {
		if (msg.level() != LEVEL || TYPE != MSG_TYPE(msg.type())) {
			throw RuntimeError{"ancillary message type mismatch"};
		}

		if (expected_size && msg.dataLength() != *expected_size) {
			throw RuntimeError{"ancillary message with length mismatch"};
		}
	}
};

/// Base class for transmit-path control message.
/**
 * This base class supports serialization of control messages of
 * MSG_TYPE::TYPE e.g. SocketMessage::TXTIME.
 **/
template <auto TYPE>
class AncillaryTxMessage {
public: // types

	using MSG_TYPE = decltype(TYPE);
	static constexpr OptLevel LEVEL = ControlMsgTraits<MSG_TYPE>::LEVEL;

public: // functions

	/// Serialize derived-type data into the given `hdr`.
	virtual void serialize(SendMessageHeader &hdr) const = 0;

protected: // functions

	/// Create a new control message in `block`.
	/**
	 * This creates a new entry in the control message payload of `block`
	 * to store `data_len` control message payload bytes. The caller is
	 * responsible for filling the buffer with the appropriate payload
	 * data.
	 *
	 * The control message level and type will be filled in automatically
	 * by this call.
	 **/
	SendMessageHeader::ControlMessage createMsg(
			SendMessageHeader::ControlMessageBlock &block,
			const size_t data_len) const {
		return block.create(LEVEL, to_integral(TYPE), data_len);
	}

	/// Provides access to the payload portion of the given `msg`.
	std::byte* data(SendMessageHeader::ControlMessage &msg) const {
		return msg.data();
	}
};

template <auto TYPE>
class AncillaryRxTxMessage :
	public AncillaryRxMessage<TYPE>,
	public AncillaryTxMessage<TYPE> {
};

/// Base class for simple ancillary receive messages containing a single scalar data type.
template <typename ITEM, auto TYPE>
class ScalarAuxRxMessage :
		public AncillaryRxMessage<TYPE> {
public: // functions

	ScalarAuxRxMessage() = default;

	explicit ScalarAuxRxMessage(const ReceiveMessageHeader::ControlMessage &msg) {
		deserialize(msg);
	}

	void deserialize(const ReceiveMessageHeader::ControlMessage &msg) override {
		this->checkMsg(msg, sizeof(m_item));

		std::memcpy(&m_item, msg.data(), sizeof(m_item));
	}

	/// Provides access to the last deserialized data item.
	const ITEM item() const {
		return m_item;
	}

protected: // data

	ITEM m_item{};
};

} // end ns
