// Linux
#include <linux/sched.h> // sched headers are needed for clone()
#include <sched.h>
#include <sys/resource.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <unistd.h>

// C++
#include <iostream>

// cosmos
#include "cosmos/algs.hxx"
#include "cosmos/error/ApiError.hxx"
#include "cosmos/error/InternalError.hxx"
#include "cosmos/error/UsageError.hxx"
#include "cosmos/formatting.hxx"
#include "cosmos/fs/FileSystem.hxx"
#include "cosmos/private/Scheduler.hxx"
#include "cosmos/proc/ChildCloner.hxx"
#include "cosmos/proc/Process.hxx"
#include "cosmos/proc/SigSet.hxx"
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

auto setupArgv(const StringVector &args) {
	CStringVector ret;

	for (const auto &arg: args) {
		ret.push_back(arg.c_str());
	}

	ret.push_back(nullptr);

	return ret;
}

void printChildError(const std::string_view context, const std::string_view error) {
	std::cerr << "[" << proc::getOwnPid() << "]" << context << ": " << error << std::endl;
}

// execute program and inherit parent's environment
void exec(CStringVector &v) {
	if (v.empty()) {
		cosmos_throw (InternalError("called with empty argument vector"));
	}

	::execvp (v[0], const_cast<char**>(v.data()));

	cosmos_throw (ApiError("execvp failed"));
}

// execute program and use override environment
void exec(CStringVector &v, CStringVector &e) {
	if (v.empty()) {
		cosmos_throw (InternalError("called with empty argument vector"));
	} else if(e.empty()) {
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

SubProc ChildCloner::run() {
	if (m_executable.empty() || m_argv.empty()) {
		cosmos_throw (UsageError(
			"attempted to run a sub process w/o specifying an executable path and/or argv0"
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
	FileNum pidfd{FileNum::INVALID};
	struct clone_args clone_args{};
	// this takes the pointer to the pidfd, as a 64 but unsigned integer
	clone_args.pidfd = reinterpret_cast<uint64_t>(&pidfd);
	clone_args.exit_signal = SIGCHLD;
	clone_args.flags = CLONE_CLEAR_SIGHAND | CLONE_PIDFD;
	ProcessID pid;

	switch ((pid = ProcessID{clone3(clone_args)})) {
	default: // parent process with child pid
		return SubProc{pid, pidfd};
	case ProcessID::INVALID: // an error occured
		cosmos_throw (ApiError());
	case ProcessID::CHILD: // the child process
		// let's do something!
		break;
	}

	try {
		postFork();

		auto argv = setupArgv(m_argv);

		if (!m_env) {
			exec(argv);
		} else {
			auto envp = setupEnv(m_env.value());
			exec(argv, envp);
		}
	} catch (const CosmosError &ce) {
		printChildError("post fork/exec", ce.what());
		// use something else than "1" which might help debugging this
		// situation a bit.
		::_exit(3);
	}

	// should never be reached
	return SubProc();
}

void ChildCloner::resetSignals() {
	/*
	 * the blocked signal mask is inherited via execve(), thus we need to
	 * initialize defaults here again.
	 */
	SigSet sigs{SigSet::filled};
	signal::unblock(sigs);
}

void ChildCloner::postFork() {
	if (m_post_fork_cb) {
		m_post_fork_cb(*this);
	}

	if (m_sched_settings) {
		try {
			std::visit([](auto &&sched_settings) {
				sched_settings.apply(ProcessID::SELF);
			}, *m_sched_settings);
		} catch(const std::exception &ex) {
			// treat this as non-critical, the process can still
			// run, even if not prioritized.
			printChildError("sched_setscheduler", ex.what());
		}
	}

	resetSignals();

	if (!m_cwd.empty()) {
		fs::changeDir(m_cwd);
	}

	redirectFD(cosmos::stdout, m_stdout);
	redirectFD(cosmos::stderr, m_stderr);
	redirectFD(cosmos::stdin, m_stdin);

	for (auto fd: m_inherit_fds) {
		fd.setCloseOnExec(false);
	}
}

void ChildCloner::redirectFD(FileDescriptor orig, FileDescriptor redirect) {
	if (redirect.invalid())
		return;

	redirect.duplicate(orig, CloseOnExec(false));
}

} // end ns

std::ostream& operator<<(std::ostream &o, const cosmos::ChildCloner &proc) {
	o << "Arguments: " << proc.getArgs() << "\n";
	if (!proc.getCWD().empty())
		o << "CWD: " << proc.getCWD() << "\n";

	return o;
}
