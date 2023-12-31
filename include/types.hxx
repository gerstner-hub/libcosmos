#pragma once

// Linux
#include <unistd.h>

namespace cosmos {

/**
 * @file
 *
 * This header contains low level OS interface types shared between multiple
 * libcosmos classes, that cannot be attributed to a specific subsystem like
 * `fs` or `proc`.
 **/

enum class UserID : uid_t {
	INVALID = static_cast<uid_t>(-1),
	ROOT    = 0
};

enum class GroupID : gid_t {
	INVALID = static_cast<gid_t>(-1),
	ROOT    = 0
};

} // end ns
