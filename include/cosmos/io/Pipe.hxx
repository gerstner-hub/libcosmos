#ifndef COSMOS_PIPE_HXX
#define COSMOS_PIPE_HXX

// linux
#include <unistd.h>

// cosmos
#include "cosmos/ostypes.hxx"

namespace cosmos
{

/**
 * \brief
 * 	Creates a unidirectional pipe communication channel
 * \details
 *	A pipe basically consists of two file descriptors, one for reading and
 *	one for writing to. To make use of it, it needs to inherited to a
 *	child process or otherwise be used e.g. as a wakeup mechanism for
 *	select() calls etc.
 **/
class Pipe
{
public: // functions

	explicit Pipe();

	~Pipe() { closeReadEnd(); closeWriteEnd(); }

	void closeReadEnd() { haveReadEnd() && close(m_read_end); }
	void closeWriteEnd() { haveWriteEnd() && close(m_write_end); }

	FileDesc readEnd() { return m_read_end; }
	FileDesc writeEnd() { return m_write_end; }

	bool haveReadEnd() const { return m_read_end != INVALID_FILE_DESC; }
	bool haveWriteEnd() const { return m_write_end != INVALID_FILE_DESC; }

	FileDesc takeReadEndOwnership()
	{
		auto ret = readEnd();
		invalidateReadEnd();
		return ret;
	}

	FileDesc takeWriteEndOwnership()
	{
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
	static size_t maxAtomicWriteSize();

protected: // functions

	void invalidateReadEnd() { m_read_end = INVALID_FILE_DESC; }
	void invalidateWriteEnd() { m_write_end = INVALID_FILE_DESC; }

protected: // data

	FileDesc m_read_end = INVALID_FILE_DESC;
	FileDesc m_write_end = INVALID_FILE_DESC;
	static size_t MAX_ATOMIC_WRITE;
};

} // end ns

#endif // inc. guard
