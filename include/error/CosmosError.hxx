#pragma once

// C++
#include <exception>
#include <source_location>
#include <string>
#include <string_view>

// cosmos
#include <cosmos/dso_export.h>
#include <cosmos/error/macros.hxx>

namespace cosmos {

//! Shorthand for the rather clunky type
using SourceLocation = std::source_location;

/*
 * NOTE:
 *
 * It would be nice to add the possibility of a function call backtrace to
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
 * investigated. Currently the problems seem not worth the gain.
 *
 * In C++23 a <stacktrace> header will be introduced that seems to provide
 * builtin support for backtraces.
 */

/// Base class for libcosmos exceptions.
/**
 * This base class carries the file, line and function contextual information
 * from where it was thrown. Furthermore it stores a dynamically allocated
 * string with optional additional runtime information.
 **/
class COSMOS_API CosmosError :
		public std::exception {
public: // functions

	explicit CosmosError(const std::string_view error_class,
				const std::string_view fixed_text = {},
				const SourceLocation &src_loc =
					SourceLocation::current()) :
			m_error_class{error_class},
			m_msg{fixed_text},
			m_src_loc{src_loc} {
	}

	/// Implementation of the std::exception interface.
	/**
	 * Returns a completely formatted message describing this error
	 * instance. The returned string is only valid for the lifetime of
	 * this object.
	 **/
	const char* what() const throw() override;

	/// Returns a shorter description of the error without verbose context.
	std::string shortWhat() const;

	/// Override the stored message to contain the given `msg`
	void setMessage(const std::string_view msg) {
		m_msg = msg;
		m_msg_generated = true;
	}

protected: // functions

	/// Append type specific error information to m_msg.
	/**
	 * This function is called by the implementation when error specific
	 * information needs to be appended to the `m_msg` string.
	 *
	 * At entry into this function `m_msg` can already contain data that
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
	/// The source location where the exception has been generated
	std::source_location m_src_loc;
};

// shorthand name
using Error = CosmosError;

} // end ns
