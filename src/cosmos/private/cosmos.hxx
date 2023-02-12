// cosmos
#include "cosmos/types.hxx"

namespace cosmos {

/// Controls automatic EINTR retry behaviour
/**
 * This type is used in some system call wrappers to control the automatic
 * restart logic on EINTR error returns.
 **/
using RestartOnIntr = NamedBool<struct restart_on_intr_t, true>;

extern RestartOnIntr auto_restart_syscalls;

} // end ns
