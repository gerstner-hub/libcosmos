#pragma once

// C++
#include <string>
#include <string_view>

// cosmos
#include "cosmos/dso_export.h"
#include "cosmos/SysString.hxx"

/**
 * @file
 *
 * File system path related helpers.
 **/

namespace cosmos::fs {

/// Takes an input path and returns a normalized version of it.
/**
 * A normalized path is an absolute path without any redundant separators
 * ('/') or relative path components (".", "..").
 *
 * To achieve this, this function eliminates redundant components and expands
 * the current working directory, if necessary. Retrieving the CWD is the only
 * potential system call performed by this function.
 *
 * Symbolic links will *not* be resolved. If you want this you can use
 * canoncalize_path() instead.
 **/
COSMOS_API std::string normalize_path(const std::string_view path);

/// Normalizes `path` and resolves any symbolic link components.
COSMOS_API std::string canonicalize_path(const SysString path);

} // end ns
