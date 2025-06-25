#pragma once

// Linux
#include <limits.h>
#include <unistd.h>

// cosmos
#include <cosmos/fs/FileDescriptor.hxx>

namespace cosmos {

/// Creates a unidirectional pipe communication channel.
/**
 * A pipe consists of two file descriptors, one for reading and one for
 * writing to. To make use of it, one end needs to be inherited to a child
 * process or otherwise be used e.g. as a wakeup mechanism for select() calls
 * etc.
 *
 * When using a pipe to communicate between processes an important aspect is
 * that all write ends need to be closed before an end-of-file is reported on
 * the read end. Therefore you need to make sure that only the necessary part
 * of the pipe is inherited to the child process to communicate with.
 *
 * The pipe file descriptors created by this class have the close-on-exec flag
 * set by default, so you need to explicitly re-enable that flag to inherit
 * the necessary end to a child process.
 *
 * This type maintains ownership of the contained file descriptors and thus is
 * a move-only type.
 **/
class COSMOS_API Pipe {
	// disallow copy/assignment
	Pipe(const Pipe&) = delete;
	Pipe& operator=(const Pipe&) = delete;

public: // functions

	/// Creates a pipe with both ends stored in the object.
	explicit Pipe();

	~Pipe() { closeReadEnd(); closeWriteEnd(); }

	Pipe(Pipe &&other) {
		*this = std::move(other);
	}

	Pipe& operator=(Pipe &&other) {
		m_read_end = other.m_read_end;
		m_write_end = other.m_write_end;
		other.invalidateReadEnd();
		other.invalidateWriteEnd();
		return *this;
	}

	void closeReadEnd() { if (haveReadEnd()) m_read_end.close(); }
	void closeWriteEnd() { if (haveWriteEnd()) m_write_end.close(); }

	FileDescriptor readEnd() { return m_read_end; }
	FileDescriptor writeEnd() { return m_write_end; }

	bool haveReadEnd() const { return m_read_end.valid(); }
	bool haveWriteEnd() const { return m_write_end.valid(); }

	/// Return the read end, passing ownership to the caller.
	/**
	 * the read end descriptor stored in the object will be invalidated
	 * and not be accessible at a later time anymore.
	 **/
	FileDescriptor takeReadEndOwnership() {
		auto ret = readEnd();
		invalidateReadEnd();
		return ret;
	}

	/// Return the write end, passing ownership to the caller.
	/**
	 * \see takeReadEndOwnership()
	 **/
	FileDescriptor takeWriteEndOwnership() {
		auto ret = writeEnd();
		invalidateWriteEnd();
		return ret;
	}

	/// Maximum number of bytes that can be transmitted over a Pipe as a single message.
	/**
	 * A pipe can maintain messages boundaries i.e. each write is returned
	 * in the same length on the read end. This is only possible up to a
	 * maximum size, however. This function returns this size.
	 **/
	static constexpr size_t maxAtomicWriteSize() {
		return MAX_ATOMIC_WRITE;
	}

protected: // functions

	void invalidateReadEnd() { m_read_end.reset(); }
	void invalidateWriteEnd() { m_write_end.reset(); }

protected: // data

	FileDescriptor m_read_end;
	FileDescriptor m_write_end;
	static constexpr size_t MAX_ATOMIC_WRITE = PIPE_BUF;
};

} // end ns
