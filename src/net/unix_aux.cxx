// C++
#include <cassert>
#include <cstring>

// Cosmos
#include "cosmos/error/UsageError.hxx"
#include "cosmos/fs/FileDescriptor.hxx"
#include "cosmos/net/unix_aux.hxx"

namespace cosmos {

void UnixRightsMessage::deserialize(const ReceiveMessageHeader::ControlMessage &msg) {
	if (const auto type = msg.asUnixMessage(); !type || *type != UnixMessage::RIGHTS) {
		cosmos_throw (RuntimeError("ancilliary message level/type mismatch"));
	}

	clearFDs();

	const uint8_t *data = reinterpret_cast<const uint8_t*>(msg.data());

	auto left = msg.dataLength();
	FileNum fd;

	while (left >= sizeof(fd)) {
		std::memcpy(&fd, data, sizeof(fd));
		data += sizeof(fd);
		left -= sizeof(fd);
		m_fds.push_back(fd);
	}

	if (!m_fds.empty()) {
		m_unclaimed_fds = true;
	}
}

SendMessageHeader::ControlMessage UnixRightsMessage::serialize() const {
	if (m_fds.empty()) {
		cosmos_throw (UsageError("Attempt to serialize empty vector of FileNum"));
	}

	SendMessageHeader::ControlMessage ret{
		OptLevel::SOCKET,
		to_integral(UnixMessage::RIGHTS),
		sizeof(FileNum) * m_fds.size()
	};

	assert (ret.dataSpace() >= sizeof(FileNum) * m_fds.size());

	auto data = ret.data();

	for (auto fd: m_fds) {
		std::memcpy(data, &fd, sizeof(fd));
		data += sizeof(fd);
	}

	return ret;
}

void UnixRightsMessage::closeUnclaimed() {
	if (m_unclaimed_fds) {
		for (auto fd: m_fds) {
			cosmos::FileDescriptor{fd}.close();
		}

		m_fds.clear();

		m_unclaimed_fds = false;
	}
}

} // end ns
