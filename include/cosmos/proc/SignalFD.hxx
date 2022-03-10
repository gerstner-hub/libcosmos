#ifndef COSMOS_SIGNALFD_HXX
#define COSMOS_SIGNALFD_HXX

// Linux
#include <sys/signalfd.h>

// cosmos
#include "cosmos/fs/FileDescriptor.hxx"
#include "cosmos/proc/Signal.hxx"
#include "cosmos/proc/SigSet.hxx"

namespace cosmos {

/// A file descriptor for receicing process signals
/**
 * A SignalFD is used for handling process signals on a file descriptor level.
 * During creation of the file descriptor the signals that the caller is
 * interested in are declared. Once one of these signals is sent the file
 * descriptor will become readable and returns SignalFD::SigInfo data
 * structure describing the event.
 *
 * As usual with signal handling you need to block the signals that you want
 * to handle synchronously via a SignalFD by calling Process::blockSignals().
 * Use the readEvent() member function to comfortably receive the signal
 * information. The underlying file descriptor can be used with common file
 * descriptor monitoring interfaces like select().
 **/
class COSMOS_API SignalFD {
public: // types

	//! data structure returned by readEvent()
	struct SigInfo : signalfd_siginfo {
		//! returns the signal number that occured
		auto getSignal() const { return Signal(ssi_signo); }

		//! returns the PID of the process that sent or caused this
		//! signal, if applicable
		auto getSenderPID() const  { return ssi_pid; }

		//! for SIGCHLD this returns the childs exit status or the
		//! signal that caused the child process to change state
		auto getChildStatus() const { return ssi_status; }
	};

public: // functions

	SignalFD() {}

	~SignalFD();

	/// immediately creates a signal FD listening on the given signals
	explicit SignalFD(const SigSet &mask) {
		create(mask);
	}

	/// immediately creates a signal FD listening on the given list of signals
	explicit SignalFD(const std::initializer_list<Signal> &siglist) {
		create(SigSet(siglist));
	}

	/// immediately creates a signal FD listening on exactly the given signal
	explicit SignalFD(const Signal &s) {
		create(SigSet({s}));
	}

	/// creates a new SignalFD
	/**
	 * if a SignalFD is already open then it will be closed first. If an
	 * error occurs creating the new SignalFD then an exception is thrown.
	 **/
	void create(const SigSet &mask);

	void close() { m_fd.close(); }

	auto valid() const { return m_fd.valid(); }

	/// change the signals the file descriptor is listening for
	/**
	 * A valid SignalFD must be opened for this to work, otherwise an
	 * exception is thrown.
	 **/
	void adjustMask(const SigSet &mask);

	//! reads the next event event from the SignalFD
	/**
	 * This is a blocking operation so you should use an efficient poll
	 * mechanism like select() to determine whether there is anything to
	 * read.
	 *
	 * If an error occurs trying to read a signal description then an
	 * exception is thrown.
	 **/
	void readEvent(SigInfo &info);

	/// returns the FileDescriptor object associated with the SignalFD
	auto raw() { return m_fd; }

protected: // data
	FileDescriptor m_fd;
};

}

#endif // inc. guard
