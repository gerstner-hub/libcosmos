#pragma once

/**
 * @file
 * This header contains exception handling related macros specific to
 * libcosmos's exception model.
 **/

/// Throws the given Exception type after information from the calling context has been added
/**
 * Usage of this macro is deprecated since introduction of C++20.
 **/
#define cosmos_throw(e) (throw e)
