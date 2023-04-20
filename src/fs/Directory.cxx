// cosmos
#include "cosmos/error/FileError.hxx"
#include "cosmos/formatting.hxx"
#include "cosmos/fs/Directory.hxx"
#include "cosmos/private/cosmos.hxx"

namespace cosmos {

Directory::~Directory() {
	const auto orig_fd = m_fd.raw();
	try {
		this->close();
	} catch (const std::exception &e) {
		noncritical_error(sprintf("%s: failed to close fd(%d)", __FUNCTION__, to_integral(orig_fd)), e);
	}
}

void Directory::open(const std::string_view path, const OpenMode mode, OpenFlags flags) {
	m_auto_close = AutoCloseFD{true};

	flags.set(OpenSettings::DIRECTORY);

	int raw_flags = flags.raw() | to_integral(mode);

	auto fd = ::open(path.data(), raw_flags, 0);

	m_fd.setFD(FileNum{fd});

	if (!isOpen()) {
		cosmos_throw (FileError(path, "open"));
	}
}

void Directory::open(const DirFD dir_fd, const std::string_view path, const OpenMode mode, OpenFlags flags) {
	m_auto_close = AutoCloseFD{true};

	flags.set(OpenSettings::DIRECTORY);

	int raw_flags = flags.raw() | to_integral(mode);

	auto fd = ::openat(to_integral(dir_fd.raw()), path.data(), raw_flags, 0);

	m_fd.setFD(FileNum{fd});

	if (!isOpen()) {
		cosmos_throw (FileError(path, "openat"));
	}
}

} // end ns
