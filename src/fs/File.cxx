// C++
#include <iostream>

// cosmos
#include "cosmos/algs.hxx"
#include "cosmos/error/FileError.hxx"
#include "cosmos/error/InternalError.hxx"
#include "cosmos/error/UsageError.hxx"
#include "cosmos/formatting.hxx"
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

void File::open(const std::string_view path, const OpenMode mode, const OpenFlags flags, const std::optional<FileMode> fmode) {
	int raw_flags = flags.raw() | static_cast<int>(mode);

	if (flags.anyOf({OpenSettings::CREATE, OpenSettings::TMPFILE}) && !fmode) {
		cosmos_throw (UsageError("the given open flags require an fmode argument"));
	}

	auto fd = ::open(path.data(), raw_flags, fmode ? to_integral(fmode.value().raw()) : 0);

	m_fd.setFD(FileNum{fd});

	if (!isOpen()) {
		cosmos_throw (FileError(path, "open"));
	}
}

} // end ns
