#ifndef COSMOS_STREAMFILE_HXX
#define COSMOS_STREAMFILE_HXX

// cosmos
#include "cosmos/fs/File.hxx"
#include "cosmos/io/StreamIO.hxx"

namespace cosmos {

/// Specialization of the File type for streaming I/O access
/**
 * Streaming I/O means that a file read/write position is maintained by the
 * operating system and data is exchanged by means of read/write operations
 * that transfer data from the current process to the file and vice versa.
 *
 * \see StreamIO
 **/
class StreamFile :
		public File,
		public StreamIO {
public: // functions

	StreamFile() :
		StreamIO{m_fd}
	{}

	StreamFile(const std::string_view path, const OpenMode mode) :
		File{path, mode, OpenFlags{OpenSettings::CLOEXEC}},
		StreamIO{m_fd}
	{}

	StreamFile(const std::string_view path, const OpenMode mode, const OpenFlags flags) :
		File{path, mode, flags},
		StreamIO{m_fd}
	{}

	StreamFile(const std::string_view path, const OpenMode mode, const OpenFlags flags, const FileMode fmode) :
		File{path, mode, flags, fmode},
		StreamIO{m_fd}
	{}

	StreamFile(FileDescriptor fd, const AutoCloseFD auto_close) :
		File{fd, auto_close},
       		StreamIO{m_fd}
	{}
};

}

#endif // inc. guard
