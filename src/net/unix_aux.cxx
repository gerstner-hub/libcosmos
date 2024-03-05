// C++
#include <cassert>
#include <cstring>

// Cosmos
#include <cosmos/error/UsageError.hxx>
#include <cosmos/fs/FileDescriptor.hxx>
#include <cosmos/net/unix_aux.hxx>

namespace cosmos {

static_assert(sizeof(UnixCredentials) == sizeof(struct ucred), "size mismatch between UnixCredentials and struct ucred!");

void UnixCredentials::setCurrentCreds() {
	pid = to_integral(cosmos::proc::get_own_pid());
	uid = to_integral(cosmos::proc::get_effective_user_id());
	gid = to_integral(cosmos::proc::get_effective_group_id());
}

void UnixRightsMessage::deserialize(const ReceiveMessageHeader::ControlMessage &msg) {
	this->checkMsg(msg, UnixMessage::RIGHTS);

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

	auto ret = this->createMsg(UnixMessage::RIGHTS, sizeof(FileNum) * m_fds.size());

	auto data = this->data(ret);

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

void UnixCredentialsMessage::deserialize(const ReceiveMessageHeader::ControlMessage &msg) {
	checkMsg(msg, UnixMessage::CREDENTIALS);

	const uint8_t *data = reinterpret_cast<const uint8_t*>(msg.data());
	const auto bytes = msg.dataLength();

	if (bytes != sizeof(m_creds)) {
		cosmos_throw (RuntimeError("SCM_CREDS message with mismatching length encountered"));
	}

	std::memcpy(&m_creds, data, bytes);
}

SendMessageHeader::ControlMessage UnixCredentialsMessage::serialize() const {
	auto ret = this->createMsg(UnixMessage::CREDENTIALS, sizeof(m_creds));

	auto data = this->data(ret);

	std::memcpy(data, &m_creds, sizeof(m_creds));

	return ret;
}

} // end ns
