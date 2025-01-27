#pragma once

// C++
#include <optional>

// cosmos
#include <cosmos/BitMask.hxx>
#include <cosmos/proc/pidfd.h>
#include <cosmos/proc/PidFD.hxx>
#include <cosmos/proc/process.hxx>
#include <cosmos/proc/signal.hxx>
#include <cosmos/proc/types.hxx>

namespace cosmos {

/// Wrapper around a PidFD.
/**
 * This wraps a PidFD just like a File object wraps a FileDescriptor. It adds
 * lifetime handling i.e. closes the PidFD when no longer needed and also
 * offers domain specific operations that can be performed on the PidFD.
 **/
class COSMOS_API ProcessFile {
public: // types

	enum class OpenFlag : unsigned int {
		NONBLOCK = PIDFD_NONBLOCK ///< open the file descriptor in non-blocking mode - proc::wait() will never block.
	};

	using OpenFlags = BitMask<OpenFlag>;

public: // functions

	/// Creates a new coupling to the given process ID.
	/**
	 * Note that creating a PidFD this way is often subject to race
	 * conditions i.e. the process with the given `pid` might be replaced
	 * by a different one than you expect.
	 *
	 * It can be safe if `pid` is a child process of the calling process
	 * and no other thread is calling any of the wait family of functions
	 * to cleanup the child process in case it exits.
	 **/
	explicit ProcessFile(const ProcessID pid, const OpenFlags flags = OpenFlags{});

	/// Wraps the given PidFD and takes ownership of it.
	/**
	 * The given `fd` will be owned by the new ProcessFile object. This
	 * means that ProcessFile will close() it if it deems this necessary.
	 *
	 * The only way currently to obtain `fd` is via proc::clone().
	 **/
	explicit ProcessFile(const PidFD fd) :
			m_fd{fd}
	{}

	ProcessFile(const ProcessFile&) = delete;
	ProcessFile& operator=(const ProcessFile&) = delete;

	ProcessFile(ProcessFile &&other) noexcept {
		*this = std::move(other);
	}

	ProcessFile& operator=(ProcessFile &&other) noexcept {
		m_fd = other.m_fd;
		other.m_fd.reset();
		return *this;
	}

	~ProcessFile();

	bool open() const {
		return m_fd.valid();
	}

	void close() {
		m_fd.close();
	}

	/// Returns the raw PidFD file descriptor.
	/**
	 * You can use this for some operations like cosmos::proc::wait().
	 * Make sure not to close the returned object, as ProcessFile is the
	 * owner of the file.
	 **/
	PidFD fd() const { return m_fd; }

	/// Send a signal to the represented process.
	void sendSignal(const Signal sig) const {
		signal::send(m_fd, sig);
	}

	/// Duplicate a file descriptor from the target process into the current process.
	/**
	 * This operation is similar to file descriptor passing over UNIX
	 * domain sockets. It doesn't require a socket connection though and
	 * also doesn't require the cooperation of the process the file
	 * descriptor is obtained from.
	 *
	 * The operation requires PTRACE_MODE_ATTACH_REALCREDS credentials
	 * though, which roughly means the target process needs to run under
	 * the same user as the current process, or the current process needs
	 * to be privileged.
	 *
	 * `targetfd` is the file descriptor number in the target process
	 * that should be duplicated into the current process.
	 *
	 * The returned file descriptor will have the close-on-exec flag set.
	 *
	 * The caller is responsible for closing the returned file descriptor
	 * at the appropriate time. It is best to wrap the file descriptor in
	 * a more specialized, managing type.
	 **/
	FileDescriptor dupFD(const FileNum targetfd) const;

	/// Wait for the child process to exit.
	/**
	 * \see proc::wait(const PidFD, const WaitFlags).
	 **/
	std::optional<ChildData> wait(const WaitFlags flags = WaitFlags{WaitFlag::WAIT_FOR_EXITED}) {
		return proc::wait(m_fd, flags);
	}

protected: // data

	PidFD m_fd;
};

} // end ns
