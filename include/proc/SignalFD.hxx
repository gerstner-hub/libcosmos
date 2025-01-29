#pragma once

// Linux
#include <sys/signalfd.h>

// cosmos
#include <cosmos/fs/FileDescriptor.hxx>
#include <cosmos/proc/SigInfo.hxx>
#include <cosmos/proc/SigSet.hxx>
#include <cosmos/proc/types.hxx>

namespace cosmos {

/// A file descriptor for receiving process signals.
/**
 * A SignalFD is used for handling process signals on file descriptor level.
 * During creation of the file descriptor the signals that the caller is
 * interested in are declared. Once one of these signals is sent to the
 * process, the file descriptor will become readable and returns the
 * SignalFD::SigInfo data structure describing the event.
 *
 * As usual with signal handling you need to block the signals that you want
 * to handle synchronously via a SignalFD by calling cosmos::signal::block().
 * Use the readEvent() member function to comfortably receive the signal
 * information. The underlying file descriptor can be used with common file
 * descriptor monitoring interfaces like Poller.
 *
 * The SignalFD mirrors the behaviour of other ways to handle signals:
 * When calling readEvent() then signals directed to the calling thread and
 * those directed to the process can be received. Signals directed at other
 * threads cannot be seen.
 *
 * The readEvent() function fills in SignalFD::Info, a data structure that is
 * very similar to the SigInfo structure, but not quite, which made it
 * necessary to model a distinct type for use with SignalFD.
 **/
class COSMOS_API SignalFD {
public: // types

	class Info;

public: // functions

	/// Creates an invalid SignalFD object
	SignalFD() {}

	~SignalFD();

	/// Creates a signal FD listening on the given signals.
	explicit SignalFD(const SigSet &mask) {
		create(mask);
	}

	/// Creates a signal FD listening on the given list of signals.
	explicit SignalFD(const std::initializer_list<Signal> siglist) {
		create(SigSet{siglist});
	}

	/// Creates a signal FD listening on exactly the given signal.
	explicit SignalFD(const Signal s) {
		create(SigSet{{s}});
	}

	// Prevent copying due to the file descriptor ownership.
	SignalFD(const SignalFD&) = delete;
	SignalFD& operator=(const SignalFD&) = delete;

	/// Creates a new SignalFD.
	/**
	 * if a SignalFD is already open then it will be closed first. If an
	 * error occurs creating the new SignalFD then an exception is thrown.
	 **/
	void create(const SigSet &mask);

	void close() { m_fd.close(); }

	auto valid() const { return m_fd.valid(); }

	/// Change the signals the file descriptor is listening for.
	/**
	 * A valid SignalFD must be opened for this to work, otherwise an
	 * exception is thrown.
	 **/
	void adjustMask(const SigSet &mask);

	/// Reads the next event event from the SignalFD.
	/**
	 * This is a blocking operation so you should use an efficient poll
	 * mechanism like select() to determine whether there is anything to
	 * read.
	 *
	 * If an error occurs trying to read a signal description then an
	 * exception is thrown.
	 **/
	void readEvent(Info &info);

	/// Returns the FileDescriptor object associated with the SignalFD.
	auto fd() { return m_fd; }

protected: // data

	FileDescriptor m_fd;
};

/// SigInfo style data structure returned by SignalFD::readEvent().
/**
 * This is mostly the same as SigInfo, but tailored towards SignalFD. The
 * underlying data structures differ too much to merge them into one on
 * libcosmos level.
 *
 * One difference between this structure and SigInfo is that SignalFDs cannot
 * be used to catch fault signals (these can only be caught by SigAction
 * signal handlers). Thus the fault signal part is missing from this
 * structure.
 **/
class COSMOS_API SignalFD::Info {
public: // types

	/* reuse the sub-structures as is */

	using Source       = SigInfo::Source;
	using ProcessCtx   = SigInfo::ProcessCtx;
	using UserSigData  = SigInfo::UserSigData;
	using QueueSigData = SigInfo::QueueSigData;
	using MsgQueueData = SigInfo::MsgQueueData;
	using TimerData    = SigInfo::TimerData;
	using ChildData    = SigInfo::ChildData;
	using SysData      = SigInfo::SysData;
	using PollData     = SigInfo::PollData;

public: // functions

	/// Creates a zero-initialized Info wrapper.
	Info() {
		clear();
	}

	/// Leaves the underlying data structure uninitialized.
	/**
	 * When Info is used as an output parameter only (the typical case)
	 * then you can invoke this constructor to avoid unnecessary
	 * zero-initialization.
	 **/
	Info(const no_init_t) {}

	const signalfd_siginfo* raw() const {
		return &m_raw;
	}

	signalfd_siginfo* raw() {
		return &m_raw;
	}

	/// Returns the signal number that occurred.
	Signal sigNr() const {
		// ssi_signo is unsigned, while in sigaction it's signed.
		return Signal{SignalNr{static_cast<int>(m_raw.ssi_signo)}};
	}

	/// Returns the source of the signal.
	/**
	 * \see SigInfo::source().
	 **/
	Source source() const;

	/// Returns whether the signal was sent from a trusted source (i.e. the kernel).
	/**
	 * \see SigInfo::isTrustedSource().
	 **/
	bool isTrustedSource() const {
		return m_raw.ssi_code >= 0;
	}

	/// Returns the Source::USER specific data.
	std::optional<const UserSigData> userSigData() const;

	/// Returns the Source::QUEUE specific data.
	std::optional<const QueueSigData> queueSigData() const;

	/// Returns the Source::MSGQ specific data.
	std::optional<const MsgQueueData> msgQueueData() const;

	/// Returns the Source::TIMER specific data.
	std::optional<const TimerData> timerData() const;

	/// Returns signal::BAD_SYS specific data.
	/**
	 * This data is only available for `sigNr() == signal::BAD_SYS`.
	 **/
	std::optional<const SysData> sysData() const;

	/// Returns signal::CHILD specific data.
	/**
	 * This data is only available for `sigNr() == signal::CHILD`.
	 **/
	std::optional<const ChildData> childData() const;

	/// Returns signal::POLL specific data.
	/**
	 * This data is only available for `sigNr() == signal::POLL`.
	 **/
	std::optional<const PollData> pollData() const;

	/// Zeroes out the low level siginfo_t data structure.
	void clear() {
		zero_object(m_raw);
	}

protected: // functions

	/// Returns an error code that is generally unused on Linux (always 0).
	/**
	 * An exception is the case of SIGSYS generated by seccomp(2) filters.
	 **/
	Errno error() const {
		return Errno{m_raw.ssi_errno};
	}

	ProcessCtx procCtx() const {
		return ProcessCtx{pid(), uid()};
	}

	ProcessID pid() const {
		// ssi_pid is unsigned while pid_t is signed
		return ProcessID{static_cast<pid_t>(m_raw.ssi_pid)};
	}

	UserID uid() const {
		return UserID{m_raw.ssi_uid};
	}

protected: // data

	signalfd_siginfo m_raw;
};

} // end ns
