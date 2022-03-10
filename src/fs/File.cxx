// stdlib
#include <iostream>

// cosmos
#include "cosmos/errors/ApiError.hxx"
#include "cosmos/errors/InternalError.hxx"
#include "cosmos/errors/UsageError.hxx"
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

void File::open(const std::string_view &path, const OpenMode &mode, const OpenFlags &flags, const std::optional<FileMode> &fmode) {
	int raw_flags = flags.get() | static_cast<int>(mode);

	if (flags.anyOf({OpenSettings::CREATE, OpenSettings::TMPFILE}) && !fmode) {
		cosmos_throw (UsageError("the given open flags required an fmode argument"));
	}

	auto fd = ::open(path.data(), raw_flags, fmode ? fmode.value().raw() : 0);

	m_fd.setFD(fd);

	if (!isOpen()) {
		// TODO: introduce specific file open error that carries that
		// problematic path
		cosmos_throw (ApiError(path));
	}
}

} // end ns
