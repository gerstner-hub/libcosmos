#ifndef COSMOS_PRIVATE_COSMOS_HXX
#define COSMOS_PRIVATE_COSMOS_HXX

// C++
#include <exception>
#include <optional>
#include <string_view>

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

/// handle a fatal error condition in libcosmos
/**
 * This function will take care of a fatal error condition that occured in
 * libcosmos. This should only be used in situations where execution cannot be
 * continued, not even with the use of exceptions. These are mostly the
 * following situations:
 *
 * - class destructors detect fatal conditions that would lead to resources
 *   leaks or otherwise cannot be resolved without specific application
 *   knowledge.
 * - move constructors or assignment operators encounter similar situations as
 *   above in the context of destructors.
 *
 * This call will not return. Provide a description of the problem in \c msg,
 * optionally a related exception in \c ex.
 **/
[[ noreturn ]] void fatal_error(
		const std::string_view msg,
		const std::exception *ex = nullptr);

} // end ns

#endif // inc. guard
