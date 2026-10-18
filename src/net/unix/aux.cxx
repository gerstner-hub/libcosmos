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
	this->checkMsg(msg);

	clear();

	auto data = msg.dataAs<std::byte>();
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
	this->checkMsg(msg, sizeof(FileNum));


	FileNum pidfd;
	std::memcpy(&pidfd, msg.data(), sizeof(pidfd));

	m_pidfd = ProcessFile{PidFD{pidfd}};
}

void UnixRightsMessage::serialize(SendMessageHeader &hdr) const {
	if (m_fds.empty()) {
		throw UsageError{"Attempt to serialize empty vector of FileNum"};
	}

	auto msg = this->createMsg(hdr.ctrl, sizeof(FileNum) * m_fds.size());
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
	checkMsg(msg, sizeof(m_creds));
	std::memcpy(&m_creds, msg.data(), sizeof(m_creds));
}

void UnixCredentialsMessage::serialize(SendMessageHeader &hdr) const {
	auto msg = this->createMsg( hdr.ctrl, sizeof(m_creds));
	auto data = this->data(msg);
	std::memcpy(data, &m_creds, sizeof(m_creds));
}

} // end ns
