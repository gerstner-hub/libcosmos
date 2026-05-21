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

/// Structure used to store Process UID credentials.
/**
 * This is used with cosmos::proc::get_creds() and cosmos::proc::set_creds()
 * to inspect or modify the calling process's user credentials.
 **/
struct UserCreds {
	UserID real_uid      = UserID::INVALID;
	UserID effective_uid = UserID::INVALID;
	UserID saved_uid     = UserID::INVALID;
};

/// Structure used to store Process GID credentials.
/**
 * This is used with cosmos::proc::get_creds() and cosmos::proc::set_creds()
 * to inspect or modify the calling process's group credentials.
 **/
struct GroupCreds {
	UserID real_gid      = UserID::INVALID;
	UserID effective_gid = UserID::INVALID;
	UserID saved_gid     = UserID::INVALID;
};

} // end ns
