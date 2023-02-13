// cosmos
#include "cosmos/algs.hxx"
#include "cosmos/error/ApiError.hxx"
#include "cosmos/error/RuntimeError.hxx"
#include "cosmos/fs/StreamFile.hxx"
#include "cosmos/private/cosmos.hxx"

namespace cosmos {

size_t StreamFile::read(void *buf, size_t length) {
	while (true) {
		auto res = ::read(to_integral(m_fd.raw()), buf, length);

		if (res < 0) {
			// transparent restart
			if (auto_restart_syscalls && getErrno() == Errno::INTERRUPTED)
				continue;
			cosmos_throw (ApiError("reading from file"));
		}

		return static_cast<size_t>(res);
	}
}

void StreamFile::readAll(void *buf, size_t length) {
	size_t res;
	while (length != 0) {
		res = read(buf, length);
		buf = reinterpret_cast<char*>(buf) + res;
		length -= res;

		if (res == 0) {
			cosmos_throw (RuntimeError("unexpected EOF"));
		}
	}
}

size_t StreamFile::write(const void *buf, size_t length) {
	while (true) {
		auto res = ::write(to_integral(m_fd.raw()), buf, length);

		if (res < 0) {
			// transparent restart
			if (auto_restart_syscalls && getErrno() == Errno::INTERRUPTED)
				continue;
			cosmos_throw (ApiError("writing to file"));
		}

		return static_cast<size_t>(res);
	}
}

void StreamFile::writeAll(const void *buf, size_t length) {
	size_t res;

	while (length != 0) {
		res = write(buf, length);
		buf = reinterpret_cast<const char*>(buf) + res;
		length -= res;
	}
}

off_t StreamFile::seek(const SeekType type, off_t off) {
	const auto res = ::lseek(to_integral(m_fd.raw()), off, to_integral(type));

	if (res == static_cast<off_t>(-1)) {
		cosmos_throw (ApiError("seeking in file"));
	}

	return res;
}

} // end ns
