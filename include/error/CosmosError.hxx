#pragma once

// C++
#include <exception>
#include <string>
#include <string_view>

// cosmos
#include "cosmos/dso_export.h"
#include "cosmos/error/macros.hxx"

namespace cosmos {

/*
 * NOTE:
 *
 * It would be nice adding the possibility for a function call backtrace to
 * the exception class. There are a bunch of obstacles to this, though:
 *
 * There exists a kind of base mechanism present in Glibc and GCC/Clang:
 *
 * - using backtrace() and backtrace_symbols() from <execinfo.h>
 * - using abi::__cxa_demangle from <cxxabi.h>
 *
 * The limitations of this are:
 *
 * - no line number / source file information
 * - symbolic names are only available if all objects file are compiled with
 *   `-rexport` and -fvisibility=default. Even then static function names
 *   cannot be resolved.
 *
 * There exists libunwind: is it is readily available and offers symbolic
 * names even without changing the compilation and linking process. It does
 * not offer symbol demangling though which needs to come from cxxabi.h
 * instead. Also it comes with a file descriptor leak, because it creates a
 * pipe pair that is never cleaned up. Also it does not offer line number of
 * source file information.
 *
 * There are lesser known libraries like libbacktrace or backward-cpp, both of
 * them are rather big and not readily available, also not very actively
 * maintained (?).
 *
 * Further "hacks" to get additional debug information would be calling
 * external tools like `addr2line` or `gdb`.
 *
 * Another problem is that adding only the option to have a backtrace can
 * considerably increase the exception object size. A std::vector<std::string>
 * would add 24 bytes to the object size. A single std::string is similarly
 * sized. It could be heap-allocated which would reduce the overhead for
 * non-use to 8 bytes but then we have to manage the pointer e.g. when
 * copying, a shared_ptr would add 16 bytes of overhead.
 *
 * Overall to implement this feature maybe implementing parts of it on foot
 * using existing code snippets from libbacktrace & friends could be
 * investiaged. Currently the problems seem not worth the gain.
 */

/// Base class for libcosmos exceptions.
/**
 * This base class carries the file, line and function contextual information
 * from where it was thrown. Furthermore it stores a dynamically allocated
 * string with optional additional runtime information.
 *
 * The cosmos_throw macro allows to transparently throw any type derived from
 * this base class, all contextual information filled in.
 *
 * Each derived type must override the raise() virtual function to allow to
 * throw the correct specialized type even when only the base class type is
 * known. The generateMsg() function can be overwritten to update the error
 * message content at the time what() is called. This allows to defer
 * expensive calculations until the time the actual exception message content
 * is accessed.
 **/
class COSMOS_API CosmosError :
		public std::exception {
public: // functions

	explicit CosmosError(const std::string_view error_class) :
		m_error_class{error_class} {}

	CosmosError(const std::string_view error_class, const std::string_view fixed_text) :
			CosmosError{error_class} {
		m_msg = fixed_text;
	}

	/// Set exception context information.
	/**
	 * This function is used by the \c cosmos_throw macro to store
	 * information about the program location where the exception was
	 * thrown.
	 **/
	CosmosError& setInfo(const char *file, const size_t line, const char *func) {
		m_line = line;
		m_file = file;
		m_func = func;

		return *this;
	}

	/// Implementation of the std::exception interface.
	/**
	 * Returns a completely formatted message describing this error
	 * instance. The returned string is only valid for the lifetime of
	 * this object.
	 **/
	const char* what() const throw() override;

	/// Throw the most specialized type of this object in the inheritance hierarchy.
	[[ noreturn ]] virtual void raise() = 0;

protected: // functions

	/// Append type specific error information to m_msg.
	/**
	 * This function is called by the implementation when error specific
	 * information needs to be appened to the the \c m_msg string.
	 *
	 * At entry into this function \c m_msg can already contain data that
	 * must not be discarded.
	 *
	 * This function will be called at most once during the lifetime of an
	 * object, and only if the error message actually needs to be
	 * generated due to a call to what().
	 **/
	virtual void generateMsg() const {};

	/// Allows to override error class to allow simpler implementation of derived types.
	void setErrorClass(const std::string_view error_class) {
		m_error_class = error_class;
	}

protected: // data

	/// Descriptive, unique error class label
	std::string_view m_error_class;
	/// Runtime generated error message
	mutable std::string m_msg;
	/// Whether m_msg has been assembled yet
	mutable bool m_msg_generated = false;
	const char *m_file = nullptr;
	const char *m_func = nullptr;
	size_t m_line = 0;
};

} // end ns
