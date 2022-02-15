#ifndef COSMOS_OSTYPES_HXX
#define COSMOS_OSTYPES_HXX

// Linux
#include <unistd.h>
#include <sys/types.h>

namespace cosmos {

/*
 * these are not part of the SubProc class, because then we'd get mutual
 * dependencies between Signal and SubProc headers.
 */
typedef pid_t ProcessID;
constexpr pid_t INVALID_PID = -1;

typedef uid_t UserID;

typedef ino_t Inode;

} // end ns

#endif // inc. agurd
