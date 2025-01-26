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

/// Type used to invoke constructors that explicitly don't zero-initialize low level data structures.
/**
 * Some system data structures like `struct sigaction` or `siginfo_t` are
 * rather large. Zero initializing them could be costly if they are only used
 * as system call output parameters, for example.
 *
 * Since not initializing data structures can be risky it is better to
 * explicitly request non-initialization. This is what this tag type is used
 * for. libcosmos types that support this offer an overloaded construct that
 * can be passed cosmos::no_init to request the non-initialization behaviour.
 **/
struct no_init_t {};
constexpr inline no_init_t no_init;

} // end ns
