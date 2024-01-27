// Linux
#include <fcntl.h>
#include <limits.h>

// Cosmos
#include "cosmos/error/ApiError.hxx"
#include "cosmos/io/Pipe.hxx"

namespace cosmos {

const size_t Pipe::MAX_ATOMIC_WRITE = PIPE_BUF;

Pipe::Pipe() {
	int ends[2];
	if (::pipe2(ends, O_CLOEXEC | O_DIRECT) != 0) {
		cosmos_throw (ApiError("pipe2()"));
	}

	m_read_end.setFD(FileNum{ends[0]});
	m_write_end.setFD(FileNum{ends[1]});
}

} // end ns
