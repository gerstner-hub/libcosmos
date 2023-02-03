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
typedef pid_t ProcessID;
constexpr pid_t INVALID_PID = -1;

typedef uid_t UserID;
typedef gid_t GroupID;

typedef ino_t Inode;

} // end ns

#endif // inc. agurd
