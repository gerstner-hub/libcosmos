#ifndef COSMOS_SIGNALFD_HXX
#define COSMOS_SIGNALFD_HXX

// Linux
#include <sys/signalfd.h>

// cosmos
#include "cosmos/fs/FileDescriptor.hxx"
#include "cosmos/proc/Signal.hxx"
#include "cosmos/proc/SigSet.hxx"

namespace cosmos {

/// A file descriptor for receiving process signals
/**
 * A SignalFD is used for handling process signals on a file descriptor level.
 * During creation of the file descriptor the signals that the caller is
 * interested in are declared. Once one of these signals is sent to the
 * process, the file descriptor will become readable and returns the
 * SignalFD::SigInfo data structure describing the event.
 *
 * As usual with signal handling you need to block the signals that you want
 * to handle synchronously via a SignalFD by calling Process::blockSignals().
 * Use the readEvent() member function to comfortably receive the signal
 * information. The underlying file descriptor can be used with common file
 * descriptor monitoring interfaces like select() or poll().
 **/
class COSMOS_API SignalFD {
public: // types

	/// Data structure returned by readEvent()
	struct SigInfo : signalfd_siginfo {
		/// Returns the signal number that occured
		auto getSignal() const { return Signal(SignalNr{static_cast<int>(ssi_signo)}); }

		/// Returns the PID of the process that sent or caused this signal, if applicable
		auto getSenderPID() const { return ProcessID{static_cast<pid_t>(ssi_pid)}; }

		/// For SIGCHLD this returns the child's exit status or the signal that caused the child process to change state
		/**
		 * if ssi_code is CLD_EXITED then this is the exit status,
		 * otherwise the signal number.
		 **/
		auto getChildStatus() const { return ssi_status; }
	};

public: // functions

	/// Creates an invalid SignalFD object
	SignalFD() {}

	~SignalFD();

	/// Creates a signal FD listening on the given signals
	explicit SignalFD(const SigSet &mask) {
		create(mask);
	}

	/// Creates a signal FD listening on the given list of signals
	explicit SignalFD(const std::initializer_list<Signal> &siglist) {
		create(SigSet(siglist));
	}

	/// Creates a signal FD listening on exactly the given signal
	explicit SignalFD(const Signal s) {
		create(SigSet({s}));
	}

	/// Creates a new SignalFD
	/**
	 * if a SignalFD is already open then it will be closed first. If an
	 * error occurs creating the new SignalFD then an exception is thrown.
	 **/
	void create(const SigSet &mask);

	void close() { m_fd.close(); }

	auto valid() const { return m_fd.valid(); }

	/// Change the signals the file descriptor is listening for
	/**
	 * A valid SignalFD must be opened for this to work, otherwise an
	 * exception is thrown.
	 **/
	void adjustMask(const SigSet &mask);

	/// Reads the next event event from the SignalFD
	/**
	 * This is a blocking operation so you should use an efficient poll
	 * mechanism like select() to determine whether there is anything to
	 * read.
	 *
	 * If an error occurs trying to read a signal description then an
	 * exception is thrown.
	 **/
	void readEvent(SigInfo &info);

	/// Returns the FileDescriptor object associated with the SignalFD
	auto raw() { return m_fd; }

protected: // data

	FileDescriptor m_fd;
};

} // end ns

#endif // inc. guard
