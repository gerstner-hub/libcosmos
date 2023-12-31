#pragma once

// C++
#include <chrono>
#include <optional>

// cosmos
#include "cosmos/proc/PidFD.hxx"
#include "cosmos/proc/WaitRes.hxx"
#include "cosmos/ostypes.hxx"

namespace cosmos {

/// Represents a child process created via ChildCloner.
/**
 * This is a lightweight sub process description returned from
 * ChildCloner::run. It is used to interact with a running sub process:
 *
 * - sending signals to it
 * - waiting for it to exit, optionally with timeout
 * - getting a pidfd for waiting for it in event driven I/O, or otherwise
 *   operate on the child (e.g. Linux namespace operations).
 *
 * This is a non-copyable move-only type. If you want to store it as a member
 * variable then use std::move().
 *
 * Each running() SubProc instance needs to be wait()ed on to free the
 * associated resources. The destructor will otherwise abort().
 **/
class COSMOS_API SubProc {
	/// disallow copy/assignment; this is a move-only type
	SubProc(const SubProc &other) = delete;
	SubProc& operator=(const SubProc &other) = delete;

public: // functions

	/// Creates an empty sub process without state.
	SubProc() = default;

	~SubProc();

	/// Implements move-semantics.
	/**
	 * This type can be moved but not copied. This is because the internal
	 * state cannot be copied, once the child process has exited all
	 * instances of the pidfd have to be invalidated.
	 *
	 * The move-semantics allow to keep a SubProc as a class member.
	 **/
	SubProc(SubProc &&other) noexcept {
		*this = std::move(other);
	}

	SubProc& operator=(SubProc &&other) noexcept;

	/// Returns whether a child process is still active.
	/**
	 * This can return \c true even if the child process already exited,
	 * in case the child process's exit status was not yet collected via
	 * wait().
	 **/
	auto running() const { return m_child_fd.valid(); }

	/// Performs a blocking wait until the child process exits.
	WaitRes wait();

	/// Wait for sub process exit within a timeout in milliseconds.
	/**
	 * \return The exit status if the child exited. Nothing if the timeout
	 * occured.
	 **/
	std::optional<WaitRes> waitTimed(const std::chrono::milliseconds max);

	/// Send the specified signal to the child process.
	void kill(const Signal signal);

	/// Returns the PID of the currently running child process or ProcessID::INVALID.
	ProcessID pid() const { return m_pid; }

	/// Returns a pidfd refering to the currently running child.
	/**
	 * This file descriptor can be used for efficiently waiting for child
	 * exit using poll() or select() APIs, see `man pidfd_open`. This
	 * somewhat breaks encapsulation, so take care not to misuse this file
	 * descriptor in a way that could break the SubProc class logic.
	 *
	 * The ownership of the file descriptor stays with the SubProc
	 * implementation. Never close this descriptor.
	 **/
	PidFD pidFD() const { return m_child_fd; }

protected: // functions

	friend class ChildCloner;

	/// Wraps the given process ID and pidfd.
	SubProc(const ProcessID pid, const PidFD pidfd) :
		m_pid{pid}, m_child_fd{pidfd}
	{}

protected: // data

	/// The pid of the child process, if any
	ProcessID m_pid = ProcessID::INVALID;

	/// Pidfd refering to the active child, if any
	PidFD m_child_fd;
};

} // end ns
