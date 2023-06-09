#ifndef COSMOS_DIRECTORY_HXX
#define COSMOS_DIRECTORY_HXX

// cosmos
#include "cosmos/fs/types.hxx"
#include "cosmos/fs/DirFD.hxx"

namespace cosmos {

/// Representation of open directory file descriptors.
/**
 * This is similar to the File class but dedicated to opening Directory nodes.
 * This class is not intended for access directory contents, use DirStream for
 * this instead.
 *
 * This type mainly exists to express situations when only directory file
 * descriptors are acceptable, like in the context of the various openat like
 * APIs.
 **/
class COSMOS_API Directory {
public: // functions

	Directory() = default;

	explicit Directory(DirFD fd, const AutoCloseFD auto_close) {
		open(fd, auto_close);
	}


	/// Open a directory by path without special flags (close-on-exec will be set).
	explicit Directory(const std::string_view path, const OpenMode mode = OpenMode::READ_ONLY) :
			Directory{path, mode, OpenFlags{OpenSettings::CLOEXEC}} {}

	/// Open a directory by path using the given mode and flags.
	/**
	 * \see open(const std::string_view, const OpenMode, const OpenFlags)
	 **/
	explicit Directory(const std::string_view path, const OpenMode mode, const OpenFlags flags) {
		open(path, mode, flags);
	}

	/// Open a directory by path relative to \c dir_fd using the given mode and default flags.
	Directory(const DirFD dir_fd, const std::string_view path, const OpenMode mode = OpenMode::READ_ONLY) :
			Directory{dir_fd, path, mode, OpenFlags{OpenSettings::CLOEXEC}} {}

	/// Open a directory by path relative to \c dir_fd using the given mode and flags.
	Directory(const DirFD dir_fd, const std::string_view path, const OpenMode mode,
			const OpenFlags flags) {
		open(dir_fd, path, mode, flags);
	}

	// Prevent copying due to the DirFD ownership.
	Directory(const Directory&) = delete;
	Directory& operator=(const Directory&) = delete;

	Directory(Directory &&other) {
		*this = std::move(other);
	}

	Directory& operator=(Directory &&other) {
		m_auto_close = other.m_auto_close;
		m_fd = other.m_fd;

		other.m_auto_close = AutoCloseFD{true};
		other.m_fd.reset();
		return *this;
	}

	virtual ~Directory();

	/// Open a directory by path without special flags (close-on-exec will be set).
	void open(const std::string_view path, const OpenMode mode = OpenMode::READ_ONLY) {
		return open(path, mode, OpenFlags{OpenSettings::CLOEXEC});
	}

	/// Open a directory by path using the given mode and flags.
	/**
	 * The OpenSettings::DIRECTORY flag will implicitly be set in \c
	 * flags, since this is required to ensure that the resulting file
	 * descriptor will refer to a directory.
	 **/
	void open(const std::string_view path, const OpenMode mode, OpenFlags flags);

	/// Open a directory by path relative to \c dir_fd using the given mode and default flags.
	void open(const DirFD dir_fd, const std::string_view path, const OpenMode mode) {
		open(dir_fd, path, mode, OpenFlags{OpenSettings::CLOEXEC});
	}

	/// Open a directory by path relative to \c dir_fd using the given mode and flags.
	void open(const DirFD dir_fd, const std::string_view path, const OpenMode mode,
			const OpenFlags flags);

	/// Takes the already open directory file descriptor fd and operators on it.
	/**
	 * The caller is responsible for invalidating \c fd, if desired, and
	 * that the file descriptor is not used in conflicting ways.
	 *
	 * The parameter \c auto_close determines whether the File object will
	 * take ownership of the file descriptor, or not. If so then the file
	 * descriptor is closed on OS level if deemed necessary by the
	 * implementation.
	 **/
	void open(DirFD fd, const AutoCloseFD auto_close) {
		m_fd = fd;
		m_auto_close = auto_close;
	}
	
	bool isOpen() const { return m_fd.valid(); }

	/// Close the current dir object.
	/**
	 * If currently no dir is open then this does nothing. If currently
	 * an external DirFD is wrapped and auto-close is not set
	 * then only the object's state will be invalidated. Otherwise the
	 * referenced file descriptor will also be closed on OS-level.
	 **/
	void close() {
		if (!isOpen())
			return;

		if (m_auto_close) {
			m_fd.close();
		} else {
			m_fd.reset();
		}

		m_auto_close = AutoCloseFD{true};
	}

	DirFD fd() const { return m_fd; }

	// \see fs::unlink_file_at()
	inline void unlinkFileAt(const std::string_view path) const;

	// \see fs::make_dir_at()
	inline void makeDirAt(const std::string_view path, const FileMode mode) const;

	// \see fs::remove_dir_at()
	inline void removeDirAt(const std::string_view path) const;

	// \see fs::read_symlink_at()
	inline std::string readSymlinkAt(const std::string_view path) const;

	// \see fs::make_symlink_at()
	inline void makeSymlinkAt(const std::string_view target, const std::string_view path) const;

protected: // data
	
	AutoCloseFD m_auto_close;
	DirFD m_fd;
};

} // end ns

// only include this here, because of dependency issues
#include "cosmos/fs/filesystem.hxx"

namespace cosmos {

void Directory::unlinkFileAt(const std::string_view path) const {
	fs::unlink_file_at(m_fd, path);
}

void Directory::makeDirAt(const std::string_view path, const FileMode mode) const {
	fs::make_dir_at(m_fd, path, mode);
}

void Directory::removeDirAt(const std::string_view path) const {
	fs::remove_dir_at(m_fd, path);
}

std::string Directory::readSymlinkAt(const std::string_view path) const {
	return fs::read_symlink_at(m_fd, path);
}

void Directory::makeSymlinkAt(const std::string_view target, const std::string_view path) const {
	fs::make_symlink_at(target, m_fd, path);
}

} // end ns

#endif // inc. guard
