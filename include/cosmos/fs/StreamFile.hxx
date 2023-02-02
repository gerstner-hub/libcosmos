#ifndef COSMOS_STREAMFILE_HXX
#define COSMOS_STREAMFILE_HXX

// Linux
#include <unistd.h>

// cosmos
#include "cosmos/fs/File.hxx"

namespace cosmos {

/// Specialization of the File type for streaming I/O access
/**
 * Streaming I/O means that a file read/write position is maintained by the
 * operating system and data is exchanged by means of read/write operations
 * that transfer data from the current process to the file and vice versa.
 *
 * This is the most common access mode for files but also somewhat
 * inefficient. In contrast e.g. memory mapped files can be more efficient.
 **/
class COSMOS_API StreamFile : public File {
public: // types

	/// Different methods for changing the file read/write position
	enum class SeekType : int {
		SET = SEEK_SET, /// Set a new absolute position
		CUR = SEEK_CUR, /// Set a position relative to the current one
		END = SEEK_END, /// Set a position relative to the end of the file
		/// Seek to a non-hole position
		/**
		 * For files with holes in them this seeks the next position
		 * containing data that is equal or greater to the provided
		 * offset.
		 **/
		DATA = SEEK_DATA,
		/// Seek to a hole position
		/**
		 * For files with holes in them this seeks the next position
		 * that is part of a hole that is equal or greater to the
		 * provided offset. The end-of-file is considered a whole in
		 * this context.
		 **/
		HOLE = SEEK_HOLE
	};
public: // functions

	StreamFile() {}

	StreamFile(const std::string_view &path, const OpenMode &mode) :
		File(path, mode, OpenFlags({OpenSettings::CLOEXEC})) {}

	StreamFile(const std::string_view &path, const OpenMode &mode, const OpenFlags &flags) :
		File(path, mode, flags) {}

	explicit StreamFile(FileDescriptor fd, const CloseFile close_fd) :
		File(fd, close_fd) {}

	/// Read up to \p length bytes from the file into \p buf
	/**
	 * An attempt is made to read data from the underlying file object and
	 * place it into \p buf. \p buf needs to be able to hold at least \p
	 * length bytes. Short reads can occur in which case less bytes will
	 * be read. The number of bytes actually read is returned from this
	 * function.
	 *
	 * A return value of zero indicates that the End-of-File has been
	 * reached and no further data can be obtained.
	 *
	 * On error conditions an exception is thrown.
	 **/
	size_t read(void *buf, size_t length);

	/// Write up to \p length bytes from \p buf into the underlying file
	/**
	 * An attempt is made to write data from the given \p buf and pass it
	 * to the underlying file object. \p buf needs to hold at least \c
	 * length bytes of data. Short writes can occur in which case less
	 * bytes will be written. The number of bytes actually written is
	 * returned from this function.
	 *
	 * On error conditions an exception is thrown.
	 **/
	size_t write(const void *buf, size_t length);

	/// Read *all* \p length bytes from the underlying file
	/**
	 * This behaves just like read() with the exception that on short
	 * reads the operation will be continued until all \c length bytes
	 * have been obtained from the file.
	 *
	 * An End-of-File condition is considered an error in this context and
	 * results in a RuntimeError exception. If the function returns
	 * normally then all \c length bytes will have been obtained.
	 **/
	void readAll(void *buf, size_t length);

	/// Write *all* \p length bytes into the underyling file
	/**
	 * This behaves just like write() with the exception that on short
	 * writes the operation will be continued until all \c length bytes
	 * have been written to the file.
	 *
	 * If the function returns normally then all \c length bytes will have
	 * been transferred.
	 **/
	void writeAll(const void *buf, size_t length);

	/// Seek to the given offset based on the given offset \p type
	off_t seek(const SeekType &type, off_t off);

	/// Seek to the given offset relative to the start of the file
	off_t seekFromStart(off_t off) { return seek(SeekType::SET, off); }
	/// Seek to the given offset relative to the current file position
	off_t seekFromCurrent(off_t off) { return seek(SeekType::CUR, off); }
	/// Seek to the given offset relative to the end of the file
	off_t seekFromEnd(off_t off) { return seek(SeekType::END, off); }

	/// Controls the auto-restart behaviour on EINTR due to signals
	/**
	 * If during read or write operations an EINTR system call result is
	 * encountered, then the implementation will transparently restart the
	 * system call if this boolean flag is set (the default). Otherwise
	 * the error is returned to the caller by means of an ApiError
	 * exception.
	 **/
	void setRestartOnIntr(const bool restart) {
		m_restart_on_intr = restart;
	}

protected: // data

	bool m_restart_on_intr = true;
};

}

#endif // inc. guard
