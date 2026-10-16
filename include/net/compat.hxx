#pragma once

// Linux
#include <sys/socket.h>

/**
 * @file
 *
 * A bunch of more recent socket-related preprocessor defines which aren't
 * readily available in current Linux distros' userspace headers yet.
 **/

#ifndef SO_RIGHTS_NOTRUNC
	// this was only added in kernel 7.3, not yet found in most userspace
	// headers
#	define SO_RIGHTS_NOTRUNC 85 /* generic UAPI value */
#endif

#ifndef SO_PASSRIGHTS
#	define SO_PASSRIGHTS 83
#endif

#ifndef SO_INQ
#	define SO_INQ 84
#endif

#ifndef SCM_INQ
#	define SCM_INQ SO_INQ
#endif

#ifndef SO_RCVPRIORITY
#	define SO_RCVPRIORITY 82
#endif

#ifndef SO_DEVMEM_DONTNEED
#	define SO_DEVMEM_DONTNEED 80
#endif

#ifndef MSG_SOCK_DEVMEM
#	define MSG_SOCK_DEVMEM 0x2000000
#endif
