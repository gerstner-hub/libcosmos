#pragma once

// cosmos
#include "cosmos/dso_export.h"
#include "cosmos/fs/FileBase.hxx"

namespace cosmos {

/// Memory based files suitable for storing of sensitive secret data.
/**
 * This type is similar to MemFile, but the file has some special properties
 * that make is suitable for storing sensitive secret data. The memory pages
 * will even we hidden from kernel space to a certain extent. The memory will
 * be locked i.e. it will never be swapped out.
 **/
class COSMOS_API SecretFile :
		public FileBase {
public: // functions

	SecretFile() = default;

	/// \c see create().
	explicit SecretFile(const CloseOnExec cloexec) {
		create(cloexec);
	}

	/// Create a new MemFile using the given settings.
	/**
	 * Create a new memory file using the given flags and optional page
	 * size. The \c name is only for debugging purposes and is used as an
	 * identifier in the /proc file system.
	 **/
	void create(const CloseOnExec cloexec = CloseOnExec{true});
};

} // end ns
