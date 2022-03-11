// stdlib
#include <cstdlib>
#include <iostream>

// Linux
#include <signal.h>
#include <sys/ptrace.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

// cosmos
#include "cosmos/errors/ApiError.hxx"
#include "cosmos/errors/InternalError.hxx"
#include "cosmos/errors/UsageError.hxx"
#include "cosmos/private/ChildCollector.hxx"
#include "cosmos/private/Scheduler.hxx"
#include "cosmos/proc/SubProc.hxx"

namespace cosmos {

static ChildCollector g_collector;

ChildCollector::ChildCollector() :
	Initable(InitPrio::CHILD_COLLECTOR)
{}

void ChildCollector::libInit() {
	// currently a prerequisite for using the ChildCollector
	// sigtimedwait() based approach
	sigset_t sigs;
	sigemptyset(&sigs);
	sigaddset(&sigs, SIGCHLD);

	if (sigprocmask(SIG_BLOCK, &sigs, nullptr) != 0) {
		cosmos_throw (cosmos::ApiError());
	}
}

void ChildCollector::libExit() {
	// restore the default block unmask for SIGCHLD
	sigset_t sigs;
	sigemptyset(&sigs);
	sigaddset(&sigs, SIGCHLD);

	if (sigprocmask(SIG_UNBLOCK, &sigs, nullptr) != 0) {
		std::cerr << "failed to unblock SIGCHLD: " << ApiError().msg() << "\n";
	}
}

SubProc::SubProc()
{}

SubProc::~SubProc() {
	if (m_pid != INVALID_PID) {
		std::cerr << "child process still running: " << m_pid << "\n";
		std::abort();
	}
}

void SubProc::run(const StringVector &sv) {
	const auto &args = sv.empty() ? m_argv : sv;

	if (args.empty()) {
		cosmos_throw (UsageError(
			"attempted to run a subprocess w/o specifying an executable path"
		));
	}

	switch ((m_pid = ::fork()))
	{
	default: // parent process with child pid
		// as documented, to prevent future inheritance of undefined
		// file descriptor state
		resetStdFiles();
		return;
	case -1: // an error occured
		// see above, same for error case
		resetStdFiles();
		cosmos_throw (ApiError());
		return;
	case 0: // the child process
		// let's do something!
		break;
	}

	try {
		postFork();

		CStringVector argv;

		for (auto &arg: args) {
			argv.push_back(arg.c_str());
		}

		argv.push_back(nullptr);

		this->exec(argv);
	}
	catch (const CosmosError &ce) {
		std::cerr
			<< "Execution of child process failed:\n"
			<< *this << "\n" << ce.what() << std::endl;
		::_exit(1);
	}
}

void SubProc::resetSignals() {
	/*
	 * the blocked signal mask is inherited via execve(), thus we need to
	 * initialize defaults here again.
	 */
	sigset_t sigs;
	sigfillset(&sigs);
	if (sigprocmask(SIG_UNBLOCK, &sigs, nullptr) != 0) {
		cosmos_throw (ApiError());
	}
}

void SubProc::postFork() {
	if (m_post_fork_cb) {
		m_post_fork_cb(*this);
	}
	if (m_sched_settings) {
		try {
			m_sched_settings->apply(0);
		}
		catch(const std::exception &ex) {
			// treat this as non-critical, the process can still
			// run, even if not prioritized.
			std::cerr
				<< "[" << getpid() << "] " << __FUNCTION__
				<< ": sched_setscheduler: " << ex.what()
				<< std::endl;
		}
	}

	resetSignals();

	if (!m_cwd.empty()) {
		if (::chdir(m_cwd.c_str()) != 0) {
			cosmos_throw (ApiError());
		}
	}

	redirectFD(cosmos::stdout, m_stdout);
	redirectFD(cosmos::stderr, m_stderr);
	redirectFD(cosmos::stdin, m_stdin);

	resetStdFiles();

	if (m_trace) {
#if 0
		// actually if we make our parent a tracer this way then we
		// can't deal with it the "new" way as possible with SEIZED
		// processes. So we only raise a SIGSTOP as below to have the
		// parent catch us before doing anything else and otherwise
		// the parent can SEIZE us.
		if (::ptrace( PTRACE_TRACEME, INVALID_PID, 0, 0 ) != 0) {
			cosmos_throw (ApiError());
		}
#endif

		// this allows our parent to wait for us such that is knows
		// we're a tracee now
		Signal::raiseSignal (Signal(SIGSTOP));
	}
}

void SubProc::redirectFD(FileDescriptor orig, FileDescriptor redirect) {
	if (redirect.invalid())
		return;

	redirect.duplicate(orig, false /* no close on-exec */);
}

void SubProc::exec(CStringVector &v) {
	if (v.empty()) {
		cosmos_throw (InternalError("called with empty argument vector"));
	}

	::execvp (v[0], const_cast<char**>(v.data()));

	cosmos_throw (ApiError("execvp failed"));
}

void SubProc::kill(const Signal &s) {
	Signal::sendSignal(m_pid, s);
}

WaitRes SubProc::wait() {
	WaitRes wr;
	auto pid = m_pid;
	m_pid = INVALID_PID;

	g_collector.collect(pid, wr);

	return wr;
}

bool SubProc::waitTimed(const size_t max_ms, WaitRes &res) {
	res.reset();
	bool exited = false;

	if (max_ms == SIZE_MAX) {
		// this conflicts with the interpretation of SIZE_MAX in
		// ChildCollector as "use no timeout".
		cosmos_throw (UsageError("max_ms parameter is too large"));
	}

	try {
		exited = g_collector.collect(m_pid, max_ms, res);
	}
	catch(...) {
		// probably the child can't be saved
		m_pid = INVALID_PID;
		throw;
	}

	if (exited) {
		m_pid = INVALID_PID;
	}

	return exited;
}

void SubProc::gone(const WaitRes &r) {
	m_pid = INVALID_PID;
}

void SubProc::reportStolenWaitRes(ProcessID pid, const WaitRes &wr) {
	g_collector.reportStolenChild(pid, wr);
}

} // end ns

std::ostream& operator<<(std::ostream &o, const cosmos::SubProc &proc) {
	o << "Subprocess PID " << proc.m_pid << "\n";
	o << "Arguments: " << proc.args() << "\n";
	if (!proc.cwd().empty())
		o << "CWD: " << proc.cwd() << "\n";
	if (!proc.trace())
		o << "Trace: " << proc.trace() << "\n";

	return o;
}
