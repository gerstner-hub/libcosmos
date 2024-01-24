#pragma once

#include "cosmos/dso_export.h"

namespace cosmos {

/**
 * @file
 *
 * This header contains the API for global library initialization and other
 * global library settings.
 **/

/// Initializes the cosmos library before first use.
/**
 * The initialization of the library is required before any other
 * functionality of libcosmos is accessed. This initialization should occur
 * after the main() function has been entered and not from within static
 * initializers to avoid issues with static initialization order in
 * executables / libraries.
 *
 * Multiple initializations can be performed but finish() needs to be
 * called the same number of times for cleanup to trigger.
 **/
void COSMOS_API init();

/// Undo initialization of init().
void COSMOS_API finish();

/// Convenience initialization object.
/**
 * During the lifetime of this object the cosmos library remains initialized.
 **/
struct Init {
	Init() { init(); }

	~Init() { finish(); }
};

/// This controls library behaviour upon EINTR returns from certain system calls.
/**
 * In some situations system calls can return prematurely with an EINTR
 * errno set. This can happen when an asynchronous signal is received in the
 * process and automatic system call restarting was not configured for the
 * signal handler (SA_RESTART). It is designed to allow userspace to react to
 * the condition of a signal being received (to e.g. shutdown or refresh
 * program state).
 *
 * Some system calls can still return EINTR even if signal handlers specify
 * SA_RESTART. Also a SIGSTOP/SIGSTOP sequence can cause EINTR to occur. See
 * man(7) signal for the details.
 *
 * libcosmos does not use asynchronous signal handling by itself since this
 * involves many complexities that are difficult to manage. Since other
 * components in the process might use asynchronous signal handling though,
 * this libraray setting allows you to choose libcosmos's behaviour should an
 * EINTR error occur. The default is to transparently ignore EINTR returns and
 * restart the system call in question. If an application wants to explicitly
 * deal with EINTR returns you can call change this setting to `false`
 * which will cause ApiError() exceptions with the appropriate
 * Errno::INTERRUPTED value to be thrown, left to be handled by the
 * application.
 *
 * This is a global setting that influences different kinds of libcosmos API
 * calls.
 **/
void COSMOS_API set_restart_syscall_on_interrupt(const bool auto_restart);

} // end ns

