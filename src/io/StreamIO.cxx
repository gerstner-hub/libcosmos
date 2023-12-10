// cosmos
#include "cosmos/algs.hxx"
#include "cosmos/error/ApiError.hxx"
#include "cosmos/error/RuntimeError.hxx"
#include "cosmos/error/WouldBlock.hxx"
#include "cosmos/io/StreamIO.hxx"
#include "cosmos/private/cosmos.hxx"

namespace cosmos {

namespace {

void handleIOError(const std::string_view operation) {
	// transparent restart, if configured
	if (const auto error = get_errno(); auto_restart_syscalls && error == Errno::INTERRUPTED)
		return;
	else if (in_list(error, {Errno::AGAIN, Errno::WOULD_BLOCK}))
		cosmos_throw (WouldBlock(operation));

	cosmos_throw (ApiError(operation));

}

} // end anons ns

size_t StreamIO::read(void *buf, size_t length) {
	while (true) {
		auto res = ::read(to_integral(m_stream_fd.raw()), buf, length);

		if (res < 0) {
			handleIOError("reading from file");
			continue;
		}

		return static_cast<size_t>(res);
	}
}

void StreamIO::readAll(void *buf, size_t length) {
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

size_t StreamIO::write(const void *buf, size_t length) {
	while (true) {
		auto res = ::write(to_integral(m_stream_fd.raw()), buf, length);

		if (res < 0) {
			handleIOError("writing to file");
			continue;
		}

		return static_cast<size_t>(res);
	}
}

void StreamIO::writeAll(const void *buf, size_t length) {
	size_t res;

	while (length != 0) {
		res = write(buf, length);
		buf = reinterpret_cast<const char*>(buf) + res;
		length -= res;
	}
}

bool StreamIO::read(ReadIOVector &iovec) {
	while (true) {
		const auto res = ::readv(
				to_integral(m_stream_fd.raw()), iovec.raw(), iovec.size());

		if (res < 0) {
			handleIOError("reading to vector from file");
			continue;
		}

		return iovec.update(static_cast<size_t>(res));
	}
}

bool StreamIO::write(WriteIOVector &iovec) {
	while (true) {
		auto res = ::writev(to_integral(m_stream_fd.raw()), iovec.raw(), iovec.size());

		if (res < 0) {
			handleIOError("writing vector to file");
			continue;
		}

		return iovec.update(static_cast<size_t>(res));
	}
}

off_t StreamIO::seek(const SeekType type, off_t off) {
	const auto res = ::lseek(to_integral(m_stream_fd.raw()), off, to_integral(type));

	if (res == static_cast<off_t>(-1)) {
		cosmos_throw (ApiError("seeking in file"));
	}

	return res;
}

template <typename MEMORY_REGION>
bool IOVector<MEMORY_REGION>::update(size_t processed_bytes) {
	// there's two approaches to update an io vector after partial
	// read/write operations:
	// a) removing completely processed entry from the begin of the
	//    vector and update partially processed ones
	// b) only updating pointer and length information but keeping
	//    the entry in the vector.
	// For b) the erase operation on the front of the vector is
	// somewhat expensive. For a) the re-entry into the kernel is
	// somewhat expensive, since the first entries processed will
	// potentially be finished already. For b) the advantage is that even
	// a fixed size std::array would be possible to use.  Currently we
	// follow b).
	bool vec_finished = true;

	for (auto &entry: *this) {
		processed_bytes -= entry.update(processed_bytes);

		if (!entry.finished()) {
			vec_finished = false;
			break;
		}
	}

	if (processed_bytes != 0) {
		cosmos_throw (RuntimeError("inconsistency while updating IOVector"));
	}

	return vec_finished;
}

} // end ns
