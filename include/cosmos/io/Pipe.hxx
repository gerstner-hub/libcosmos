#ifndef COSMOS_PIPE_HXX
#define COSMOS_PIPE_HXX

// Linux
#include <unistd.h>

// cosmos
#include "cosmos/fs/FileDescriptor.hxx"

namespace cosmos {

/**
 * \brief
 * 	Creates a unidirectional pipe communication channel
 * \details
 *	A pipe basically consists of two file descriptors, one for reading and
 *	one for writing to. To make use of it, it needs to inherited to a
 *	child process or otherwise be used e.g. as a wakeup mechanism for
 *	select() calls etc.
 **/
class COSMOS_API Pipe {
public: // functions

	explicit Pipe();

	~Pipe() { closeReadEnd(); closeWriteEnd(); }

	void closeReadEnd() { if (haveReadEnd()) m_read_end.close(); }
	void closeWriteEnd() { if (haveWriteEnd()) m_write_end.close(); }

	FileDescriptor readEnd() { return m_read_end; }
	FileDescriptor writeEnd() { return m_write_end; }

	bool haveReadEnd() const { return m_read_end.valid(); }
	bool haveWriteEnd() const { return m_write_end.valid(); }

	FileDescriptor takeReadEndOwnership() {
		auto ret = readEnd();
		invalidateReadEnd();
		return ret;
	}

	FileDescriptor takeWriteEndOwnership() {
		auto ret = writeEnd();
		invalidateWriteEnd();
		return ret;
	}

	/**
	 * \brief
	 *	Maximum number of bytes that can be transmitted over a Pipe as
	 *	a single message
	 * \details
	 *	A pipe can maintain messages boundaries i.e. each write is
	 *	returned in the same length on the read end. This is only
	 *	possible up to a maximum size, however. This function returns
	 *	this size.
	 **/
	static size_t maxAtomicWriteSize() {
		return MAX_ATOMIC_WRITE;
	}

protected: // functions

	void invalidateReadEnd() { m_read_end.reset(); }
	void invalidateWriteEnd() { m_write_end.reset(); }

protected: // data

	FileDescriptor m_read_end;
	FileDescriptor m_write_end;
	static const size_t MAX_ATOMIC_WRITE;
};

} // end ns

#endif // inc. guard
