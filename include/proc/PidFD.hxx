#pragma once

// cosmos
#include "cosmos/fs/FileDescriptor.hxx"

namespace cosmos {

/// A specialized FileDescriptor for pidfds.
/**
 * A file descriptor representing a process in the system. These can be used
 * to refer to or interact with other processes in the system in a race-free
 * fashion (compared to accessing /proc or specifying ProcessIDs, for example).
 *
 * This is just a thin wrapper around the file descriptor that does not offer
 * specific operations or lifetime management. Use cosmos::ProcessFile for
 * this.
 *
 * A PidFD can be obtained via cosmos::Proc or from proc::clone(). The
 * uses of a PidFD are the following:
 *
 * - send a signal to the represented process
 * - monitor process termination using (e)poll or select, i.e.
 *   cosmos::ChildCloner. There is a limitation: the file descriptor will
 *   appear as readable in the poll API but it won't actually return any data.
 * - it can be waited on using proc::wait(), but only if the process is a
 *   child of the calling process.
 * - it can be used to obtain a file descriptor from the represented process,
 *   see proc::getFD().
 * - it can be used to enter any namespaces of the target process using
 *   setns().
 * - it can be used with process_madvise() to inform the kernel about memory
 *   usage patterns of the target process.
 **/
class COSMOS_API PidFD :
		public FileDescriptor {

	friend struct CloneArgs;
public: // functions

	explicit PidFD(FileNum fd = FileNum::INVALID) :
			FileDescriptor{fd}
	{}

protected: // data
};

} // end ns
