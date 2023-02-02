// stdlib
#include <cstdlib>
#include <iostream>

// Linux
#include <linux/sched.h>
#include <sched.h>
#include <signal.h>
#include <sys/ptrace.h>
#include <sys/resource.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

// cosmos
#include "cosmos/errors/ApiError.hxx"
#include "cosmos/errors/InternalError.hxx"
#include "cosmos/errors/UsageError.hxx"
#include "cosmos/io/Poller.hxx"
#include "cosmos/private/Scheduler.hxx"
#include "cosmos/proc/SubProc.hxx"

namespace cosmos {

namespace {

// creates a vector of string pointers suitable to pass as envp to execve() and friends
auto setupEnv(const StringVector &vars) {
	CStringVector ret;

	for (const auto &var: vars) {
		ret.push_back(var.c_str());
	}

	ret.push_back(nullptr);

	return ret;
}

void exec(CStringVector &v) {
	if (v.empty()) {
		cosmos_throw (InternalError("called with empty argument vector"));
	}

	::execvp (v[0], const_cast<char**>(v.data()));

	cosmos_throw (ApiError("execvp failed"));
}

void exec(CStringVector &v, CStringVector &e) {
	if (v.empty()) {
		cosmos_throw (InternalError("called with empty argument vector"));
	}
	else if(e.empty()) {
		// needs to contain at least a terminating nullptr
		cosmos_throw (InternalError("called with empty environment vector"));
	}

	::execvpe (v[0], const_cast<char**>(v.data()), const_cast<char**>(e.data()));

	cosmos_throw (ApiError("execvpe failed"));
}

// for clone(3) there is no glibc wrapper yet so we need to wrap it ourselves
pid_t clone3(struct clone_args &args) {
	return syscall(SYS_clone3, &args, sizeof(args));
}

} // end anon ns

SubProc::SubProc()
{}

SubProc::~SubProc() {
	if (m_child_fd.valid()) {
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

	// use clone3() instead of fork():
	//
	// clone() allows us to get a pid fd for the child in a race free
	// fashion
	//
	// clone3() has fork() semantics and is easier to use for this case
	// than the older clone syscalls.
	//
	// NOTE: it turns out that clone3() is not yet supported in Valgrind,
	// meaning we won't be able to run programs through Valgrind any more
	// that employ this system call. clone2() is annoying to use because
	// it doesn't have fork() semantics though ... maybe we can sit this
	// out until Valgrind gets supports for clone3().
	int pfd = 0;
	struct clone_args clone_args{};
	clone_args.pidfd = reinterpret_cast<uint64_t>(&pfd);
	clone_args.exit_signal = SIGCHLD;
	clone_args.flags = CLONE_CLEAR_SIGHAND | CLONE_PIDFD;

	switch ((m_pid = clone3(clone_args))) {
	default: // parent process with child pid
		// as documented, to prevent future inheritance of undefined
		// file descriptor state
		resetStdFiles();
		m_child_fd.setFD(pfd);
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

		if (!m_env) {
			exec(argv);
		} else {
			auto envp = setupEnv(m_env.value());
			exec(argv, envp);
		}

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

	redirect.duplicate(orig, CloseOnExec(false));
}

void SubProc::kill(const Signal &s) {
	Signal::sendSignal(m_child_fd, s);
}

// seems also not part of userspace headers yet
// this is actually an enum, even worse ...
#ifndef P_PIDFD
#	define P_PIDFD 3
#endif

WaitRes SubProc::wait() {
	WaitRes wr;

	m_pid = INVALID_PID;

	if (waitid((idtype_t)P_PIDFD, m_child_fd.raw(), &wr, WEXITED) != 0) {
		try {
			m_child_fd.close();
		} catch(...) {}
		cosmos_throw (ApiError());
	}

	m_child_fd.close();

	return wr;
}

std::optional<WaitRes> SubProc::waitTimed(const std::chrono::milliseconds &max) {

	Poller poller(8);
	poller.addFD(m_child_fd, Poller::MonitorMask(Poller::MonitorSetting::INPUT));
	if (poller.wait(max).empty()) {
		return std::nullopt;
	}

	return wait();
}

} // end ns

std::ostream& operator<<(std::ostream &o, const cosmos::SubProc &proc) {
	o << "Subprocess PID " << proc.m_pid << "\n";
	o << "Arguments: " << proc.args() << "\n";
	if (!proc.cwd().empty())
		o << "CWD: " << proc.cwd() << "\n";
	if (!proc.getTrace())
		o << "Trace: " << proc.getTrace() << "\n";

	return o;
}
