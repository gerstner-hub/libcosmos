#pragma once

// cosmos
#include <cosmos/fs/FileDescriptor.hxx>
#include <cosmos/io/StreamIO.hxx>

namespace cosmos {

/// Base class for File types with ownership of a FileDescriptor.
/**
 * Implementations of FileBase carry ownership of a FileDescriptor. How this
 * FileDescriptor is obtained is defined by specializations of this class.
 *
 * This type implements the file close() logic and the moveable-only-semantics
 * i.e. the type is non-copiable, but the ownership of the file descriptor can
 * be transferred to other instances using std::move() and implicit move
 * operations.
 *
 * This type inherits the StreamIO interface for operating on the file content
 * using streaming file I/O.
 **/
class COSMOS_API FileBase :
		public StreamIO {
protected: // functions

	FileBase(const FileDescriptor fd = FileDescriptor{}) :
		StreamIO{m_fd},
		m_fd{fd}
	{}

	// support move semantics, but only protected, to prevent move object
	// slicing. derived classes need to provide their own move
	// ctors/operators that invoke the base class ones.

	FileBase(FileBase &&other) :
			StreamIO{m_fd} {
		*this = std::move(other);
	}

	FileBase& operator=(FileBase &&other) {
		m_fd = other.m_fd;
		other.m_fd.reset();

		return *this;
	}


public: // functions

	virtual ~FileBase();

	// Prevent copying due to the file descriptor ownership.
	FileBase(const FileBase&) = delete;
	FileBase& operator=(const FileBase&) = delete;

	/// Close the current file object.
	/**
	 * If currently no file is open then this does nothing. If currently
	 * an external FileDescriptor is wrapped and auto-close is not set
	 * then only the object's state will be invalidated. Otherwise the
	 * referenced file descriptor will also be closed on OS-level.
	 **/
	virtual void close() {
		if (!isOpen())
			return;

		m_fd.close();
	}

	/// Returns whether currently a FileDescriptor is opened.
	bool isOpen() const { return m_fd.valid(); }

	/// Allows access to the underlying fd with const semantics.
	FileDescriptor fd() const { return m_fd; }

protected: // data

	FileDescriptor m_fd;
};

} // end ns
