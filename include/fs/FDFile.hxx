#pragma once

// cosmos
#include <cosmos/fs/FileBase.hxx>

namespace cosmos {

/// File objects that are opened from existing FileDescriptor objects.
/**
 * This is a thin file type that simply wraps an existing FileDescriptor
 * object. Taking ownership of the provided file descriptor is optional and
 * needs to be decided explicitly. If ownership is not taken then the file
 * descriptor will never be closed by the implementation.
 **/
class COSMOS_API FDFile :
		public FileBase {
public: // functions

	FDFile() = default;

	/// Wrap the given file descriptor applying the specified auto-close behaviour.
	FDFile(const FileDescriptor fd, const AutoCloseFD auto_close) {
		open(fd, auto_close);
	}

	FDFile(FDFile &&other) {
		*this = std::move(other);
	}

	FDFile& operator=(FDFile &&other) {
		m_auto_close = other.m_auto_close;
		other.m_auto_close = AutoCloseFD{true};

		FileBase::operator=(std::move(other));
		return *this;
	}

	~FDFile();

	/// Takes the already open file descriptor fd and operates on it.
	/**
	 * The caller is responsible for invalidating `fd`, if desired, and
	 * that the file descriptor is not used in conflicting ways.
	 *
	 * The parameter `auto_close` determines whether the File object
	 * will take ownership of the file descriptor, or not. If so then the
	 * file descriptor is closed on OS level if deemed necessary by the
	 * implementation.
	 **/
	void open(const FileDescriptor fd, const AutoCloseFD auto_close) {
		m_fd = fd;
		m_auto_close = auto_close;
	}

	/// Close the current file object.
	/**
	 * If currently no file is open then this does nothing. If currently
	 * an external FileDescriptor is wrapped and auto-close is not set
	 * then only the object's state will be invalidated. Otherwise the
	 * referenced file descriptor will also be closed on OS-level.
	 **/
	void close() override {

		if (!m_auto_close) {
			m_fd.reset();
			m_auto_close = AutoCloseFD{true};
			return;
		}

		FileBase::close();
	}

protected: // data
	AutoCloseFD m_auto_close;
};

} // end ns
