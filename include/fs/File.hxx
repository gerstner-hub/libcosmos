#ifndef COSMOS_NAMEDFILE_HXX
#define COSMOS_NAMEDFILE_HXX

// C++
#include <optional>
#include <string_view>

// cosmos
#include "cosmos/fs/FDFile.hxx"
#include "cosmos/fs/DirFD.hxx"

namespace cosmos {

/// File objects that are opened by pathname.
/**
 * On the level of this type the means to open a file by pathname are
 * provided. This also allows to create new files depending on the used
 * OpenFlags. As a special case opening file by name relative to an existing
 * DirFD is possible.
 *
 * This is the typical File type to use when there are no special
 * circumstances.
 **/
class COSMOS_API File :
		public FDFile {
public: // functions

	File() = default;

	/// Open a file without special flags (close-on-exec will be set).
	File(const std::string_view path, const OpenMode mode) :
			File{path, mode, OpenFlags{OpenFlag::CLOEXEC}} {}

	/// Open a file using specific OpenFlags, potentially creating it first using the given \c fmode.
	/**
	 * \warning If used for creating a file, then you need to specify
	 * also the FileMode in that case. An exception will the thrown if
	 * this condition is violated.
	 **/
	File(const std::string_view path, const OpenMode mode, const OpenFlags flags,
			const std::optional<FileMode> fmode = {}) {
		open(path, mode, flags, fmode);
	}

	/// Open the given path relative to the given directory file descriptor \c dir_fd.
	/**
	 * \see open(const DirFD, const std::string_view, const OpenFlags, const std::optional<FileMode>).
	 **/
	File(const DirFD dir_fd, const std::string_view path, const OpenMode mode,
			const OpenFlags flags, const std::optional<FileMode> fmode = {}) {
		open(dir_fd, path, mode, flags, fmode);
	}

	/// Wrap the given file descriptor applying the specified auto-close behaviour.
	File(const FileDescriptor fd, const AutoCloseFD auto_close) :
		FDFile{fd, auto_close}
	{}

	using FDFile::open;

	/// \see File(const std::string_view, const OpenMode).
	void open(const std::string_view path, const OpenMode mode) {
		return open(path, mode, {OpenFlag::CLOEXEC});
	}

	/// Open the given path applying the specified mode and flags.
	/**
	 * If in \p flags the #CREATE bit is set then you \b must specify also
	 * \p fmode, otherwise an exception is thrown.
	 **/
	void open(const std::string_view path, const OpenMode mode,
			const OpenFlags flags, const std::optional<FileMode> fmode = {});

	/// \see open(const DirFD, const std::string_view, const OpenMode, const OpenFlags, const std::optional<FileMode>).
	void open(const DirFD dir_fd, const std::string_view path, const OpenMode mode) {
		open(dir_fd, path, mode, {OpenFlag::CLOEXEC});
	}

	/// Open the given path relative to the given directory file descriptor \c dir_fd.
	/**
	 * \see fs::open_at().
	 **/
	void open(const DirFD dir_fd, const std::string_view path, const OpenMode mode,
			const OpenFlags flags, const std::optional<FileMode> fmode = {});
};

} // end ns

#endif // inc. guard
