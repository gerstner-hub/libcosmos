// Linux
#include <sched.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <unistd.h>

// C++
#include <iostream>

// cosmos
#include <cosmos/error/InternalError.hxx>
#include <cosmos/error/UsageError.hxx>
#include <cosmos/formatting.hxx>
#include <cosmos/fs/filesystem.hxx>
#include <cosmos/io/EventFile.hxx>
#include <cosmos/private/cosmos.hxx>
#include <cosmos/private/Scheduler.hxx>
#include <cosmos/proc/ChildCloner.hxx>
#include <cosmos/proc/clone.hxx>
#include <cosmos/proc/ProcessFile.hxx>
#include <cosmos/proc/process.hxx>
#include <cosmos/proc/SigSet.hxx>
#include <cosmos/proc/SubProc.hxx>
#include <cosmos/proc/types.hxx>
#include <cosmos/utils.hxx>

namespace cosmos {

namespace {

// ProcessFile with the ability to take ownership of the contained FD
class UnsafeProcessFile :
		public ProcessFile {
public:
	using ProcessFile::ProcessFile;

	void invalidate() {
		m_fd.reset();
	}
};


// creates a vector of string pointers suitable to pass as envp to execve() and friends
auto setup_env(const StringVector &vars) {
	CStringVector ret;

	for (const auto &var: vars) {
		ret.push_back(var.c_str());
	}

	ret.push_back(nullptr);

	return ret;
}

auto setup_argv(const StringVector &args) {
	CStringVector ret;

	for (const auto &arg: args) {
		ret.push_back(arg.c_str());
	}

	ret.push_back(nullptr);

	return ret;
}

void print_child_error(const std::string_view context, const std::string_view error) {
	std::cerr << "[" << proc::get_own_pid() << "]" << context << ": " << error << std::endl;
}

} // end anon ns

SubProc ChildCloner::run() {
	if (m_allow_no_exe) {
		if (!m_post_fork_cb) {
			cosmos_throw (UsageError(
				"attempted to run a cloned process without callback function (no post fork CB)"
			));
		}
	} else if (m_executable.empty() || m_argv.empty()) {
		cosmos_throw (UsageError(
			"attempted to run a sub process w/o specifying an executable path and/or argv0"
		));
	}

	if (!cosmos::running_on_valgrind) {
		if (auto subproc = runClone3(); subproc) {
			return std::move(*subproc);
		}
	} else {
		if (auto subproc = runClone2(); subproc) {
			return std::move(*subproc);
		}
	}

	// the child process -- let's do something!
	runChild();
	// should never be reached
	return SubProc{};
}

std::optional<SubProc> ChildCloner::runClone2() {
	// An alternative is using regular fork() and creating a
	// pidfd from the child PID. As long as no one is collecting
	// the child status in parallel in another thread via any of
	// the wait() functions this is race free.
	//
	// This has some hairy error situations, though:
	//
	// - creating the pidfd after fork() can fail, in this case we
	//   cannot fulfill the API and need to end the child process
	//   again.
	// - the child process may exit before we have a chance to
	//   create the pidfd. Basically a pidfd can also be obtained
	//   for a zombie child process. But this is not possible if
	//   the SIGCHLD handler is set so SIG_IGN. Since we cannot
	//   guarantee this in a general purpose library we need to
	//   deal with the worst case.
	//
	// To cover these situations we use an EventFile. The child
	// process will not exec() before the parent process signals.
	// If something goes wrong in the parent process then the
	// child process is killed.

	EventFile ev;

	if (auto child = proc::fork(); child != std::nullopt) {
		auto pid = *child;
		try {
			auto pidfile = UnsafeProcessFile{pid};
			auto pidfd = pidfile.fd();
			ev.signal();
			pidfile.invalidate();
			return SubProc{pid, pidfd};
		} catch (...) {
			try {
				signal::send(pid, signal::KILL);
			} catch (const std::exception &ex) {
				std::cerr << "WARNING: failed to kill half-ready child process\n";
			}
			(void)proc::wait(pid);
			throw;
		}
	} else {
		try {
			// wait for the parent to signal us to
			// continue to exec()
			ev.wait();
		} catch (const std::exception &ex) {
			print_child_error("post fork/ev wait", ex.what());
			proc::exit(ExitStatus{3});
		}
	}

	return std::nullopt;
}

std::optional<SubProc> ChildCloner::runClone3() {
	/*
	 * use clone3() instead of fork():
	 *
	 * clone() allows us to get a pid fd for the child in a race
	 * free fashion
	 *
	 * clone3() has fork() semantics and is easier to use for this
	 * case than the older clone syscalls.
	 *
	 * NOTE: clone3() is not yet supported in Valgrind, which
	 * means that is isn't possible to run programs that employ
	 * this system call through Valgrind anymore. clone2() is
	 * annoying to use because it doesn't have fork() semantics
	 * though ... maybe we can sit this out until Valgrind gets
	 * support for clone3().
	 */
	PidFD pidfd;
	CloneArgs clone_args;
	clone_args.setPidFD(pidfd);
	clone_args.setFlags({CloneFlag::CLEAR_SIGHAND, CloneFlag::PIDFD});

	if (auto pid = proc::clone(clone_args); pid != std::nullopt) {
		// parent process with child pid
		return SubProc{*pid, pidfd};
	} else {
		return std::nullopt;
	}
}

void ChildCloner::runChild() {
	try {
		postFork();

		if (m_allow_no_exe) {
			proc::exit(ExitStatus(0));
		}

		auto argv = setup_argv(m_argv);

		if (!m_env) {
			proc::exec(argv[0], &argv);
		} else {
			auto envp = setup_env(m_env.value());
			proc::exec(argv[0], &argv, &envp);
		}
	} catch (const CosmosError &ce) {
		print_child_error("post fork/exec", ce.what());
		// use something else than "1" which might help debugging this
		// situation a bit.
		proc::exit(ExitStatus{3});
	}
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
	if (m_sched_settings) {
		try {
			std::visit([](auto &&sched_settings) {
				sched_settings.apply(ProcessID::SELF);
			}, *m_sched_settings);
		} catch(const std::exception &ex) {
			// treat this as non-critical, the process can still
			// run, even if not prioritized.
			print_child_error("sched_setscheduler", ex.what());
		}
	}

	resetSignals();

	if (!m_cwd.empty()) {
		fs::change_dir(m_cwd);
	}

	redirectFD(cosmos::stdout, m_stdout);
	redirectFD(cosmos::stderr, m_stderr);
	redirectFD(cosmos::stdin, m_stdin);

	for (auto fd: m_inherit_fds) {
		fd.setCloseOnExec(false);
	}

	if (m_post_fork_cb) {
		m_post_fork_cb(*this);
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
