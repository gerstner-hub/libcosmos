#ifndef COSMOS_STREAMIO_HXX
#define COSMOS_STREAMIO_HXX

// Linux
#include <unistd.h>

// C++
#include <string>
#include <string_view>

// cosmos
#include "cosmos/fs/FileDescriptor.hxx"

namespace cosmos {

/// Wrapper around file descriptors for streaming I/O access
/**
 * Streaming I/O means that a file read/write position is maintained by the
 * operating system and data is exchanged by means of read/write operations
 * that transfer data from the current process to the file and vice versa.
 *
 * This is the most common access mode for files but also somewhat
 * inefficient. In contrast e.g. memory mapped files can be more efficient.
 *
 * Some special devices of file types may also support streaming I/O access.
 * This type can also be used with them - but be sure to understand the
 * special I/O semantics for the respective file type when using it with this
 * wrapper.
 *
 * This type will not take ownership of the provided file descriptor. It is
 * only meant as an access wrapper, not as a permanent representation of the
 * backed file.
 **/
class COSMOS_API StreamIO {
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

	explicit StreamIO(FileDescriptor &fd) : m_stream_fd{fd}
	{}

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

	/// string_view wrapper around write(const void*, size_t)
	size_t write(const std::string_view data) {
		return write(data.data(), data.size());
	}

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

	/// Like readAll(void*, size_t) using an STL string.
	void readAll(std::string &s, size_t length) {
		s.resize(length);
		try {
			readAll(s.data(), length);
		} catch(...) {
			s.clear();
			throw;
		}
	}

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

	/// string_view wrapper around writeAll(const void*, size_t)
	void writeAll(const std::string_view data) {
		return writeAll(data.data(), data.size());
	}

	/// Seek to the given offset based on the given offset \p type
	off_t seek(const SeekType type, off_t off);

	/// Seek to the given offset relative to the start of the file
	off_t seekFromStart(off_t off) { return seek(SeekType::SET, off); }
	/// Seek to the given offset relative to the current file position
	off_t seekFromCurrent(off_t off) { return seek(SeekType::CUR, off); }
	/// Seek to the given offset relative to the end of the file
	off_t seekFromEnd(off_t off) { return seek(SeekType::END, off); }
protected: // data
	FileDescriptor &m_stream_fd;
};

} // end ns

#endif // inc. guard
