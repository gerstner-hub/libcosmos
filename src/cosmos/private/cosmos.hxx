#pragma once

// C++
#include <exception>
#include <optional>
#include <string_view>

// cosmos
#include <cosmos/utils.hxx>

namespace cosmos {

/// Controls automatic EINTR retry behaviour.
/**
 * This type is used in some system call wrappers to control the automatic
 * restart logic on EINTR error returns.
 **/
using RestartOnIntr = NamedBool<struct restart_on_intr_t, true>;

extern RestartOnIntr auto_restart_syscalls;

/// Indicates whether a Valgrind virtual execution environment was detected.
/**
 * The cosmos init function tries to find out if Valgrind is running and then
 * sets this to `true`. This flag is used in some spots to enable fallback to
 * some new system calls that are not supported by Valgrind yet, like
 * `clone3()` in ChildCloner.
 **/
using RunningOnValgrind = NamedBool<struct running_on_valgrind_t, false>;

extern RunningOnValgrind running_on_valgrind;

/// handle a fatal error condition in libcosmos.
/**
 * This function will take care of a fatal error condition that occurred in
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
 * This call will not return. The process will be terminated after writing
 * error context to stderr. Provide a description of the problem in `msg`,
 * optionally a related exception in `ex`.
 **/
[[ noreturn ]] void fatal_error(
		const std::string_view msg,
		const std::exception *ex = nullptr);

/// handle a noncritical library error that cannot be turned into an exception.
/**
 * This function will take care of recoverable error conditions that cannot be
 * expressed in form of exceptions, because they occur e.g. in an object's
 * destructor (from where exceptions shouldn't ever be thrown)..
 *
 * This can be used e.g. when a close operation for a resource like a file
 * descriptor fails. This is a rare event and even an application might not
 * know what to do about it. If an application want to deal with the
 * possibility of this happening then it can call an object's close method
 * explicitly, catching possible exceptions. Otherwise this function call here
 * will provide information about the condition on stderr and execution will
 * continue - possibly leaving a small resource leak.
 **/
void noncritical_error(
		const std::string_view msg,
		const std::exception &ex);

} // end ns
