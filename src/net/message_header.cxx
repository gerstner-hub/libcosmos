// cosmos
#include <cosmos/error/RuntimeError.hxx>
#include <cosmos/net/message_header.hxx>

namespace cosmos {

static_assert(
	sizeof(struct cmsghdr) == sizeof(ReceiveMessageHeader::ControlMessage),
	"size mismatch between cmsghdr vs. struct ReceiveMessageHeader::ControlMessage in system headers");

void SendMessageHeader::setAddress(const SocketAddress &addr) {
	m_header.msg_name = const_cast<void*>(reinterpret_cast<const void*>(addr.basePtr()));
	m_header.msg_namelen = addr.size();
}

SendMessageHeader::ControlMessage SendMessageHeader::ControlMessageBlock::create(
		const OptLevel level, int type, const size_t data_len) {

	const auto new_len = m_buffer.size() + CMSG_SPACE(data_len);
	m_buffer.reserve(new_len);

	struct cmsghdr *next_hdr = reinterpret_cast<struct cmsghdr*>(
			m_buffer.data() + m_buffer.size());

	m_buffer.resize(new_len);

	next_hdr->cmsg_level = to_integral(level);
	next_hdr->cmsg_type = type;
	next_hdr->cmsg_len = CMSG_LEN(data_len);

	return ControlMessage{next_hdr, data_len};
}

SendMessageHeader::ControlMessage::ControlMessage(
		struct cmsghdr *hdr, const size_t data_len) :
		m_header{hdr}, m_len{data_len} {
}

void SendMessageHeader::prepareSend(const SocketAddress *addr) {
	if (addr) {
		setAddress(*addr);
	} else {
		resetAddress();
	}

	setIov(iovec);

	if (ctrl.empty()) {
		m_header.msg_control = nullptr;
		m_header.msg_controllen = 0;
	} else {
		m_header.msg_control = const_cast<std::byte*>(ctrl.raw());
		m_header.msg_controllen = ctrl.size();
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
		throw RuntimeError{"control buffer size smaller than control message header"};
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
