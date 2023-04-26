#ifndef COSMOS_LOCALE_HXX
#define COSMOS_LOCALE_HXX

// libc
#include <locale.h>

// C++
#include <string>
#include <string_view>

// cosmos
#include "cosmos/dso_export.h"

namespace cosmos::locale {

/**
 * @file
 *
 * This header contains locale specific types and functionality.
 *
 * Beware that the following functions are not thread-safe (by C-API design)
 *
 * Setting up the locale should be done in the main thread of a program early
 * on so that this poses no problem.
 **/

/// Different Locale Categories that can be configured.
enum class Category : int {
	ALL            = LC_ALL,       /// all aspects of the locale
	COLLATE        = LC_COLLATE,   /// comparison of strings
	CTYPE          = LC_CTYPE,     /// character classification (e.g. alphanumeric, numeric, ...)
	MESSAGES       = LC_MESSAGES,  /// natural language messages
	MONETARY       = LC_MONETARY,  /// formatting of monetary values
	NUMERIC        = LC_NUMERIC,   /// non-monetary numeric values
	TIME           = LC_TIME       /// formatting of date and time values
#ifdef LC_ADDRESS /* the following are GNU extensions */
	,ADDRESS       = LC_ADDRESS,        /// formatting of addresses and geography related items
	IDENTIFICATION = LC_IDENTIFICATION, /// metadata about a locale
	MEASUREMENT    = LC_MEASUREMENT,    /// measurement settings (e.g. metric vs. US)
	NAME           = LC_NAME,           /// salutations for persons
	PAPER          = LC_PAPER,          /// standard paper size
	TELEPHONE      = LC_TELEPHONE       /// formats for telephone services
#endif
};

/// Returns a string describing the currently active locale setting for the given category.
COSMOS_API std::string get(Category category);

/// Set the given locale category to the given value.
/**
 * This may throw an ApiError if the request cannot be honored.
 **/
COSMOS_API void set(Category category, const std::string_view val);

/// Set the given locale category to its default value ("C" or "POSIX").
COSMOS_API void set_to_default(Category category);

/// Set the given locale category according to present environment variables.
/**
 * This function call will inspect the environment variables and set the given
 * locale category accordingly (`setlocale(cat, "")`.
 **/
COSMOS_API void set_from_environment(Category category);

} // end ns

#endif // inc. guard
