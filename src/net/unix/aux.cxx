// C++
#include <cassert>
#include <cstring>

// Cosmos
#include <cosmos/error/ParseError.hxx>
#include <cosmos/error/UsageError.hxx>
#include <cosmos/fs/FileDescriptor.hxx>
#include <cosmos/net/unix/aux.hxx>

namespace cosmos {

static_assert(sizeof(UnixCredentials) == sizeof(struct ucred), "size mismatch between UnixCredentials and struct ucred!");

void UnixCredentials::setCurrentCreds() {
	pid = to_integral(cosmos::proc::get_own_pid());
	uid = to_integral(cosmos::proc::get_effective_user_id());
	gid = to_integral(cosmos::proc::get_effective_group_id());
}

void UnixRightsMessage::deserialize(
		const ReceiveMessageHeader::ControlMessage &msg) {
	this->checkMsg(msg, UnixMessage::RIGHTS);

	clear();

	const uint8_t *data = reinterpret_cast<const uint8_t*>(msg.data());
	auto left = msg.dataLength();
	FileNum fd;

	while (left >= sizeof(fd)) {
		std::memcpy(&fd, data, sizeof(fd));
		data += sizeof(fd);
		left -= sizeof(fd);
		if (to_integral(fd) >= 0) {
			m_fds.push_back(fd);
		} else {
			/* must be the SO_RIGHTS_NOTRUNC option in effect */
			m_errors.push_back(Errno{to_integral(fd)});
		}
	}

	if (!m_fds.empty()) {
		m_unclaimed_fds = true;
	}
}

void PidFdMessage::deserialize(
		const ReceiveMessageHeader::ControlMessage &msg) {
	m_pidfd.close();
	this->checkMsg(msg, UnixMessage::PIDFD);


	const uint8_t *data = reinterpret_cast<const uint8_t*>(msg.data());

	if (msg.dataLength() != sizeof(FileNum)) {
		throw ParseError{"size mismatch in SCM_PIDFD ancillary message"};
	}

	FileNum pidfd;

	std::memcpy(&pidfd, data, sizeof(pidfd));

	m_pidfd = ProcessFile{PidFD{pidfd}};
}

void UnixRightsMessage::serialize(SendMessageHeader &hdr) const {
	if (m_fds.empty()) {
		throw UsageError{"Attempt to serialize empty vector of FileNum"};
	}

	auto msg = this->createMsg(hdr.ctrl, UnixMessage::RIGHTS, sizeof(FileNum) * m_fds.size());

	auto data = this->data(msg);

	for (auto fd: m_fds) {
		std::memcpy(data, &fd, sizeof(fd));
		data += sizeof(fd);
	}
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

void UnixCredentialsMessage::deserialize(const ReceiveMessageHeader::ControlMessage &msg) {
	checkMsg(msg, UnixMessage::CREDENTIALS);

	const uint8_t *data = reinterpret_cast<const uint8_t*>(msg.data());
	const auto bytes = msg.dataLength();

	if (bytes != sizeof(m_creds)) {
		throw RuntimeError{"SCM_CREDS message with mismatching length encountered"};
	}

	std::memcpy(&m_creds, data, bytes);
}

void UnixCredentialsMessage::serialize(SendMessageHeader &hdr) const {
	auto msg = this->createMsg(
			hdr.ctrl,
			UnixMessage::CREDENTIALS,
			sizeof(m_creds));

	auto data = this->data(msg);

	std::memcpy(data, &m_creds, sizeof(m_creds));
}

} // end ns
