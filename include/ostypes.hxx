#ifndef COSMOS_OSTYPES_HXX
#define COSMOS_OSTYPES_HXX

// Linux
#include <unistd.h>
#include <sys/types.h>

namespace cosmos {

/**
 * @file
 *
 * This header contains low level OS interface types shared between multiple
 * libcosmos classes.
 **/

/*
 * these are not part of the SubProc class, lest we end up in mutual
 * dependencies between Signal and SubProc headers.
 */
enum class ProcessID : pid_t {
	INVALID = -1,
	/// in a number of system calls zero refers to the calling thread
	SELF = 0,
	/// in fork/clone like system calls zero refers to the child context
	CHILD = 0
};

typedef uid_t UserID;
typedef gid_t GroupID;

typedef ino_t Inode;

} // end ns

#endif // inc. agurd
