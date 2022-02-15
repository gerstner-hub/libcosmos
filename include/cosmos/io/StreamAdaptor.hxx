#ifndef COSMOS_STREAMADAPTOR_HXX
#define COSMOS_STREAMADAPTOR_HXX

// Cosmos
#include "cosmos/compiler.hxx"
#include "cosmos/types.hxx"
#include "cosmos/fs/FileDescriptor.hxx"
#include "cosmos/io/Pipe.hxx"
#include "cosmos/errors/UsageError.hxx"


// stdlib
#include <iostream>
#ifdef COSMOS_GNU_CXXLIB
#include <ext/stdio_filebuf.h>
#else
/*
 * this currently only works with libstdc++, since the StdioFileBuf is
 * libstdc++ specific. For other standard libraries other hacks may exist
 * which aren't covered yet.
 */
#	error "Only GNU libstdc++ is supported right now"
#endif

namespace cosmos {

/**
 * \brief
 *	GNU libstdc++ proprietary type for wrapping already open file
 *	descriptors
 **/
typedef __gnu_cxx::stdio_filebuf<char> StdioFileBuf;

/**
 * \brief
 *	Generic template base class for wrapping existing file descriptors in
 *	C++ streams
 * \details
 *	The standardized stream library does not offer an option to wrap an
 *	existing file descriptor in a stream object. One would need to
 *	implement a complete custom stream buffer type for this which is quite
 *	overkill.
 *
 *	Most standard libraries already offer some proprietary interface since
 *	they need to do the same thing anyways for the std::cout, std::cerr
 *	objects.
 **/
template <typename STREAM_TYPE>
class StreamAdaptor :
	public STREAM_TYPE
{
public: // functions

	/**
	 * \brief
	 *	Close the underlying file descriptor
	 **/
	virtual void close() {
		m_buffer.close();
	}

	FileDescriptor fileDesc() { return FileDescriptor(m_buffer.fd()); }

protected: // functions

	StreamAdaptor(FileDescriptor fd, std::ios_base::openmode mode) :
		m_buffer(fd.raw(), mode)
	{
		if (fd.invalid()) {
			cosmos_throw (UsageError("Attempt to construct StreamAdaptor for invalid FD"));
		}

		this->rdbuf(&m_buffer);
	}

protected: // data
	StdioFileBuf m_buffer;
};

class InputStreamAdaptor :
	public StreamAdaptor<std::istream>
{
public: // functions

	explicit InputStreamAdaptor(FileDescriptor fd) :
		StreamAdaptor<std::istream>(fd, std::ios_base::in)
	{}

	explicit InputStreamAdaptor(Pipe &p) :
		InputStreamAdaptor(p.takeReadEndOwnership())
	{}
};

class OutputStreamAdaptor :
	public StreamAdaptor<std::ostream>
{
public: // functions

	explicit OutputStreamAdaptor(FileDescriptor fd) :
		StreamAdaptor<std::ostream>(fd, std::ios_base::out)
	{}

	explicit OutputStreamAdaptor(Pipe &p) :
		OutputStreamAdaptor(p.takeWriteEndOwnership())
	{}

	void close() override {
		*this << std::flush;
		StreamAdaptor::close();
	}
};

class InputOutputStreamAdaptor :
	public StreamAdaptor<std::iostream>
{
public: // functions

	explicit InputOutputStreamAdaptor(FileDescriptor fd) :
		StreamAdaptor<std::iostream>(
			fd, std::ios_base::in | std::ios_base::out
		)
	{}

	void close() override
	{
		*this << std::flush;
		StreamAdaptor::close();
	}
};

} // end ns

#endif // inc. guard
