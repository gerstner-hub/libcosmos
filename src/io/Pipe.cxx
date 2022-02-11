// Linux
#include <fcntl.h>
#include <limits.h>

// Cosmos
#include "cosmos/errors/ApiError.hxx"
#include "cosmos/io/Pipe.hxx"

namespace cosmos
{

const size_t Pipe::MAX_ATOMIC_WRITE = PIPE_BUF;

Pipe::Pipe()
{
	int ends[2];
	if( pipe2(ends, O_CLOEXEC | O_DIRECT) != 0 )
	{
		cosmos_throw( ApiError() );
	}

	m_read_end = ends[0];
	m_write_end = ends[1];
}

} // end ns
