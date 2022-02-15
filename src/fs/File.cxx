// stdlib
#include <iostream>

// cosmos
#include "cosmos/errors/ApiError.hxx"
#include "cosmos/errors/InternalError.hxx"
#include "cosmos/fs/File.hxx"

namespace cosmos {

File::~File() {
	const auto orig_fd = m_fd.raw();
	try {
		this->close();
	} catch (const std::exception &e) {
		std::cerr << __FUNCTION__ << ": fd(" << orig_fd << "): " << e.what() << std::endl;
	}
}

void File::open(const char *path, const OpenMode &mode, const OpenFlags &flags) {
	int raw_flags = flags.get() | static_cast<int>(mode);

	if (flags.test(OpenSettings::CREATE)) {
		cosmos_throw (InternalError("open with O_CREAT not yet covered"));
	}

	auto fd = ::open(path, raw_flags, 0);

	m_fd.setFD(fd);

	if (!isOpen()) {
		// TODO: introduce specific file open error that carries that
		// problematic path
		cosmos_throw (ApiError(path));
	}
}

} // end ns
