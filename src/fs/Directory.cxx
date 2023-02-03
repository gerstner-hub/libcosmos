// Cosmos
#include "cosmos/fs/Directory.hxx"

// Linux
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

namespace cosmos {

void Directory::close() {
	if (!m_stream) {
		return;
	}

	auto ret = closedir(m_stream);
	m_stream = nullptr;

	if (ret == -1) {
		cosmos_throw (ApiError());
	}
}

void Directory::open(const std::string_view &path, const FollowSymlinks follow_links) {
	close();

	/*
	 * reuse the open(FileDescriptor) logic and only open the file
	 * descriptor before. This allows us to pass needful flags like
	 * O_CLOEXEC. This gives us more control than when using opendir().
	 */
	auto res = ::open(
		path.data(),
		O_RDONLY | O_CLOEXEC | O_DIRECTORY | (follow_links ? O_NOFOLLOW : 0)
	);

	FileDescriptor fd(res);

	if (fd.invalid()) {
		cosmos_throw (ApiError());
	}

	try {
		open(fd);
	}
	catch (...) {
		// intentionally ignore error conditions here
		try {
			fd.close();
		} catch(...) {}
		throw;
	}
}

void Directory::open(const FileDescriptor &fd) {
	close();

	m_stream = fdopendir(fd.raw());

	if (!m_stream) {
		cosmos_throw (ApiError());
	}
}

FileDescriptor Directory::fd() const {
	requireOpenStream(__FUNCTION__);
	auto fd = dirfd(m_stream);
	FileDescriptor ret(fd);

	if (ret.invalid()) {
		cosmos_throw (ApiError());
	}

	return ret;
}

DirEntry Directory::nextEntry()
{
	requireOpenStream(__FUNCTION__);

	/*
	 * there's a bit confusion between readdir and readdir_r(). Today on
	 * Linux readdir() is thread-safe between different directory streams
	 * but not thread safe when using the same directory stream in
	 * parallel. The latter is rather peculiar and should not be needed.
	 * Therefore use readdir().
	 */

	// needed to differentiate between end-of-stream and error condition
	errno = 0;
	auto entry = readdir(m_stream);

	if (entry) {
		return DirEntry(entry);
	}

	if (errno != 0) {
		cosmos_throw( ApiError() );
	}

	return DirEntry(nullptr);
}

} // end ns
