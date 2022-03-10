#ifndef COSMOS_FILESYSTEM_HXX
#define COSMOS_FILESYSTEM_HXX

// C++ stdlib
#include <string_view>

namespace cosmos {

/**
 * \brief
 * 	Access to various file system information
 **/
class COSMOS_API FileSystem
{
public: // functions

	/**
	 * \brief
	 * 	Returns whether the given file system object exists
	 * \details
	 * 	The information returned is only a snapshot in time. It is
	 * 	subject to race conditions i.e. when trying to open the file
	 * 	afterwards it may already be gone or have been replaced by a
	 * 	different file system object.
	 *
	 * 	For safely testing for existence and opening a file, you
	 * 	should open the file right away and test for an according
	 * 	error condition.
	 *
	 * 	This function will not follow symlinks i.e. if \c path refers
	 * 	to a dangling symlink then it will still return \c true.
	 *
	 * 	If the condition can't be exactly determined, because an error
	 * 	different than "ENOENT" is returned by the operating system
	 * 	then an exception is thrown.
	 **/
	static bool existsFile(const std::string_view &path);
};

} // end ns

#endif // inc. guard
