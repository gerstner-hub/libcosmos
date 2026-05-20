#pragma once

// Linux
#include <unistd.h>

namespace cosmos {

/**
 * @file
 *
 * This header contains types related to process and file system object
 * credentials like user IDs, group IDs etc.
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
