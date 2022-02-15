#ifndef COSMOS_DIRECTORY_HXX
#define COSMOS_DIRECTORY_HXX

// stdlib
#include <exception>
#include <iostream>

// Linux
#include <dirent.h>

// cosmos
#include "cosmos/errors/ApiError.hxx"
#include "cosmos/errors/UsageError.hxx"
#include "cosmos/fs/DirEntry.hxx"
#include "cosmos/ostypes.hxx"

namespace cosmos {

/**
 * \brief
 * 	Access directory contents in the file system
 * \details
 * 	Using this type you can open directories in the file system either by
 * 	path or by using an already opened file descriptor. The directory
 * 	contents can then be iterated over.
 *
 * 	Note that the directory contents will be returned by the operating
 * 	system in an undefined order (i.e. not alphabetically or otherwise
 * 	sorted). Also entries for "." and ".." by convention should show
 * 	up and often need to be filtered by applications.
 **/
class COSMOS_API Directory
{
public: // types

	typedef long DirPos;

public: // functions

	/**
	 * \brief
	 *	Create a Directory using the given file descriptor or an
	 *	unassociated Directory
	 * \see
	 * 	open(FileDesc fd)
	 **/
	explicit Directory(FileDesc fd = INVALID_FILE_DESC) {
		if (fd != INVALID_FILE_DESC)
			open(fd);
	}

	/**
	 * \brief
	 * 	Create a Directory object operating on the directory present
	 * 	at the given path location in the file system
	 **/
	explicit Directory(const std::string &path) {
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

	/**
	 * \brief
	 * 	Close the currently open directory
	 * \details
	 * 	This will disassociate the Directory object and further
	 * 	attempts to iterate over directory contents will fail.
	 *
	 * 	If closing causes an error then an exception is thrown, but
	 * 	the state of the Directory object will be invalidated, to
	 * 	avoid recurring errors trying to close() or reuse the object.
	 *
	 * 	If currently the Directory object is not associated with a
	 * 	directory then a call to this function does nothing.
	 **/
	void close();

	/**
	 * \brief
	 * 	Associate with the directory represented by the given file
	 * 	descriptor
	 * \details
	 * 	The Directory object takes over ownership of the file
	 * 	descriptor. You must not modify the file descriptor's state,
	 * 	otherwise the usage of the Directory object will become
	 * 	undefined. Also during close() the file descriptor will be
	 * 	closed by the Directory object.
	 *
	 * 	If currently the Directory object is already associated with
	 * 	another directory then this previous association will be
	 * 	implicitly close()'d.
	 **/
	void open(FileDesc fd);

	/**
	 * \brief
	 * 	Associate with the directory represented by the given file
	 * 	system path locaton
	 * \details
	 * 	If currently the Directory object is already associated with
	 * 	another directory then this previous associaton will be
	 * 	implicitly close()'d.
	 **/
	void open(const std::string &path, const bool follow_links = false);

	/**
	 * \brief
	 * 	Indicates whether currently a directory is associated with
	 * 	this object
	 **/
	auto isOpen() const { return m_stream != nullptr; }

	/**
	 * \brief
	 * 	Return the file descriptor associated with the current
	 * 	Directory object
	 * \details
	 * 	The caller must not modify the state of this file descriptor,
	 * 	otherwise further attempts to iterate over directory contents
	 * 	will result in undefined behaviour. The file descriptor will
	 * 	become invalid after close() is invoked.
	 **/
	FileDesc fd() const;

	/**
	 * \brief
	 * 	Returns the current position in the directory iteration
	 * \details
	 * 	The returned value needs to be treated opaque, i.e. no
	 * 	assumptions should be made about it. It can merely be used to
	 * 	seek() at a later point in time.
	 **/
	DirPos tell() const {
		requireOpenStream("tell");

		auto ret = telldir(m_stream);

		if (ret == -1) {
			cosmos_throw (ApiError());
		}

		return ret;
	}

	/**
	 * \brief
	 * 	Adjust the directory iterator to the position \c pos
	 * \details
	 * 	\c pos needs to be previously obtained from tell()
	 **/
	void seek(const DirPos &pos) {
		requireOpenStream("seek");

		seekdir(m_stream, pos);
	}

	/**
	 * \brief
	 * 	Returns the next entry in the associated directory
	 * \details
	 * 	Calls to this function are only allowed if isOpen() returns \c
	 * 	true. The validity of the returned object is tied to the
	 * 	lifetime of the Directory instance it came from. Also any call
	 * 	to nextEntry() will invalidate previously returned DirEntry
	 * 	instances returned from the same Directory instance.
	 *
	 * 	When the end of the directory has been reached then an invalid
	 * 	DirEntry object is returned.
	 **/
	DirEntry nextEntry();

protected: // functions

	void requireOpenStream(const char *context) const {
		if (!isOpen()) {
			cosmos_throw (UsageError(std::string(context) + " on unassociated Directory"));
		}
	}

protected: // data

	DIR *m_stream = nullptr;
};

} // end ns

#endif // inc. guard
