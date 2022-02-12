// stdlib
#include <iostream>

// cosmos
#include "cosmos/errors/ApiError.hxx"
#include "cosmos/errors/InternalError.hxx"
#include "cosmos/fs/File.hxx"

namespace cosmos {

File::~File() {
	if (isOpen()) {
		auto orig_fd = m_fd;
		try {
			close();
		} catch (const std::exception &e) {
			std::cerr << __FUNCTION__ << ": fd(" << orig_fd << "): " << e.what() << std::endl;
		}
	}
}

void File::close() {
	if (!isOpen())
		return;

	// make sure the local file descriptor is invalidated in any case to
	// prevent infinite error loops trying to close the same FD
	auto fd = m_fd;
	m_fd = INVALID_FILE_DESC;

	if (::close(fd) == 0)
		return;

	cosmos_throw (ApiError("closing file"));
}

void File::open(const char *path, const OpenMode &mode, const OpenFlags &flags) {
	int raw_flags = flags.get() | static_cast<int>(mode);

	if (flags.test(OpenSettings::CREATE)) {
		cosmos_throw (InternalError("open with O_CREAT not yet covered"));
	}

	m_fd = ::open(path, raw_flags, 0);

	if (!isOpen()) {
		// TODO: introduce specific file open error that carries that
		// problematic path
		cosmos_throw (ApiError(path));
	}
}

} // end ns
