#ifndef COSMOS_DIRECTORY_HXX
#define COSMOS_DIRECTORY_HXX

// stdlib
#include <exception>
#include <iostream>
#include <optional>
#include <string_view>

// Linux
#include <dirent.h>

// cosmos
#include "cosmos/algs.hxx"
#include "cosmos/errors/ApiError.hxx"
#include "cosmos/errors/UsageError.hxx"
#include "cosmos/fs/DirEntry.hxx"
#include "cosmos/fs/DirFD.hxx"
#include "cosmos/fs/FileDescriptor.hxx"
#include "cosmos/fs/FileSystem.hxx"

namespace cosmos {

/// Access directory contents in the file system
/**
 * Using this type you can open directories in the file system either by path
 * or by using an already opened file descriptor. The directory contents can
 * then be iterated over.
 *
 * Note that the directory contents will be returned by the operating system
 * in an undefined order (i.e. not alphabetically or otherwise sorted). Also
 * entries for "." and ".." by convention should show up and often need to be
 * filtered by applications.
 **/
class COSMOS_API Directory {
public: // functions

	/// Creates an object no associated to a directory
	Directory() = default;

	/// Create a Directory using the given file descriptor
	/**
	 * \see open(FileDescriptor fd)
	 **/
	explicit Directory(const FileDescriptor fd) {
		open(fd);
	}

	/// Create a Directory object operating on the directory at the given path location
	explicit Directory(const std::string_view path) {
		open(path);
	}

	~Directory() {
		try {
			close();
		}
		catch (const std::exception &ex) {
			// ignore otherwise
			std::cerr << __FUNCTION__
				<< ": failed to close Directory stream: "
				<< ex.what() << "\n";
		}
	}

	/// Close the currently associated directory
	/**
	 * This will disassociate the Directory object and further attempts to
	 * iterate over directory contents will fail.
	 *
	 * If closing causes an error then an exception is thrown, but the
	 * state of the Directory object will be invalidated, to avoid
	 * recurring errors trying to close() or reuse the object.
	 *
	 * If the object is not currently associated with a directory then a
	 * call to this function does nothing.
	 **/
	void close();

	/// Associate with the directory represented by the given file descriptor
	/**
	 * The implementation takes ownership of the file descriptor. You must
	 * not modify the file descriptor's state, otherwise the usage of the
	 * Directory object will become undefined. Also during close() the
	 * file descriptor will be closed by the Directory object.
	 *
	 * If the object is already associated with another directory then
	 * this previous association will be implicitly close()'d.
	 **/
	void open(const FileDescriptor fd);

	/// Associate with the directory at the given file system path locaton
	/**
	 * If the object is already associated with another directory then this
	 * previous associaton will be implicitly close()'d.
	 **/
	void open(const std::string_view path, const FollowSymlinks follow_links = FollowSymlinks(false));

	/// Indicates whether currently a directory is associated with this object
	auto isOpen() const { return m_stream != nullptr; }

	/// Return the file descriptor associated with the current Directory object
	/**
	 * The caller must not modify the state of this file descriptor,
	 * otherwise further attempts to iterate over directory contents will
	 * result in undefined behaviour. The file descriptor will become
	 * invalid after close() is invoked.
	 **/
	DirFD fd() const;

	/// Returns the current position in the directory iteration
	/**
	 * The returned value needs to be treated opaque, i.e. no assumptions
	 * should be made about it. It can merely be used to seek() at a later
	 * point in time.
	 **/
	DirEntry::DirPos tell() const {
		requireOpenStream("tell");

		auto ret = telldir(m_stream);

		if (ret == -1) {
			cosmos_throw (ApiError());
		}

		return DirEntry::DirPos{ret};
	}

	/// Adjust the directory iterator to the given position
	/**
	 * \c pos needs to be previously obtained from tell().
	 **/
	void seek(const DirEntry::DirPos pos) {
		requireOpenStream("seek");

		seekdir(m_stream, to_integral(pos));
	}

	/// Returns the next entry in the associated directory
	/**
	 * Calls to this function are only allowed if isOpen() returns \c
	 * true. The validity of the returned object is tied to the lifetime
	 * of the Directory instance it came from. Also any call to
	 * nextEntry() will invalidate previously returned DirEntry instances
	 * returned from the same Directory instance.
	 *
	 * When the end of the directory has been reached then \c nullopt is
	 * returned.
	 **/
	std::optional<DirEntry> nextEntry();

protected: // functions

	void requireOpenStream(const std::string_view context) const {
		if (!isOpen()) {
			cosmos_throw (UsageError(std::string(context) + " on unassociated Directory instance"));
		}
	}

protected: // data

	DIR *m_stream = nullptr;
};

} // end ns

// make DirIterator available for range based for loops
#include "cosmos/fs/DirIterator.hxx"

#endif // inc. guard
