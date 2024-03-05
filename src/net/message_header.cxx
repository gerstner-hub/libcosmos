// Cosmos
#include <cosmos/error/RuntimeError.hxx>
#include <cosmos/net/message_header.hxx>

namespace cosmos {

static_assert(sizeof(struct msghdr_const) == sizeof(struct msghdr),
		"size mismatch between msghdr_const vs. struct msghdr in system headers");
static_assert(sizeof(struct cmsghdr) == sizeof(ReceiveMessageHeader::ControlMessage),
		"size mismatch between cmsghdr vs. struct ReceiveMessageHeader::ControlMessage in system headers");

void SendMessageHeader::setAddress(const SocketAddress &addr) {
	m_header.msg_name = reinterpret_cast<const void*>(addr.basePtr());
	m_header.msg_namelen = addr.size();
}

SendMessageHeader::ControlMessage::ControlMessage(const OptLevel level, int type, const size_t data_len) {
	m_buffer.resize(CMSG_SPACE(data_len));
	m_header = reinterpret_cast<cmsghdr*>(m_buffer.data());
	m_header->cmsg_level = to_integral(level);
	m_header->cmsg_type = type;
	m_header->cmsg_len = CMSG_LEN(data_len);
}

void SendMessageHeader::prepareSend(const SocketAddress *addr) {
	if (addr) {
		setAddress(*addr);
	} else {
		resetAddress();
	}

	setIov(iovec);

	if (control_msg) {
		m_header.msg_control = control_msg->raw();
		m_header.msg_controllen = control_msg->size();
	} else {
		m_header.msg_control = nullptr;
		m_header.msg_controllen = 0;
	}
}

void ReceiveMessageHeader::setAddress(SocketAddress &addr) {
	m_header.msg_name = reinterpret_cast<void*>(addr.basePtr());
	m_header.msg_namelen = addr.maxSize();
}

void ReceiveMessageHeader::setControlBufferSize(const size_t bytes) {
	if (bytes == 0) {
		m_control_buffer.clear();
		return;
	} else if (bytes < sizeof(cmsghdr)) {
		cosmos_throw (RuntimeError("control buffer size smaller than control message header"));
	}

	m_control_buffer.resize(bytes);
}

void ReceiveMessageHeader::prepareReceive(SocketAddress *addr) {
	if (addr) {
		setAddress(*addr);
	} else {
		resetAddress();
	}

	setIov(iovec);

	if (!m_control_buffer.empty()) {
		m_header.msg_control = m_control_buffer.data();
		m_header.msg_controllen = m_control_buffer.size();
	}
}

} // end ns
