// cosmos
#include "cosmos/errors/ApiError.hxx"
#include "cosmos/fs/FileSystem.hxx"

// Linux
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

namespace cosmos::fs {

bool existsFile(const std::string_view path) {
	struct stat s;
	if (::lstat(path.data(), &s) == 0)
		return true;
	else if (getErrno() != Errno::NO_ENTRY)
		cosmos_throw (ApiError());

	return false;
}

void changeDir(const std::string_view path) {
	if (::chdir(path.data()) != 0) {
		cosmos_throw (ApiError());
	}
}

} // end ns
