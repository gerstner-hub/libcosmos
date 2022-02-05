#ifndef COSMOS_STREAMADAPTOR_HXX
#define COSMOS_STREAMADAPTOR_HXX

// Cosmos
#include "cosmos/compiler.hxx"
#include "cosmos/types.hxx"
#include "cosmos/io/Pipe.hxx"
#include "cosmos/errors/UsageError.hxx"

#ifndef COSMOS_GNU_CXXLIB
/*
 * this currently only works with libstdc++, since the StdioFileBuf is
 * libstdc++ specific. For other standard libraries other hacks may exist
 * which aren't covered yet.
 */
#	error "Only GNU libstdc++ is supported right now"
#endif

// C++
#include <iostream>
#include <ext/stdio_filebuf.h>

namespace cosmos
{

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
	virtual void close()
	{
		m_buffer.close();
	}

	FileDesc fileDesc() { return m_buffer.fd(); }

protected: // functions

	StreamAdaptor(FileDesc fd, std::ios_base::openmode mode) :
		m_buffer(fd, mode)
	{
		if( fd == INVALID_FILE_DESC )
		{
			cosmos_throw( UsageError("Construct StreamAdaptor for invalid FD") );
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

	explicit InputStreamAdaptor(FileDesc fd) :
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

	explicit OutputStreamAdaptor(FileDesc fd) :
		StreamAdaptor<std::ostream>(fd, std::ios_base::out)
	{}

	explicit OutputStreamAdaptor(Pipe &p) :
		OutputStreamAdaptor(p.takeWriteEndOwnership())
	{}

	void close() override
	{
		*this << std::flush;
		StreamAdaptor::close();
	}
};

class InputOutputStreamAdaptor :
	public StreamAdaptor<std::iostream>
{
public: // functions

	explicit InputOutputStreamAdaptor(FileDesc fd) :
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
