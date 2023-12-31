// Linux
#include <sched.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <unistd.h>

// C++
#include <iostream>

// cosmos
#include "cosmos/error/ApiError.hxx"
#include "cosmos/error/InternalError.hxx"
#include "cosmos/error/UsageError.hxx"
#include "cosmos/formatting.hxx"
#include "cosmos/fs/filesystem.hxx"
#include "cosmos/private/Scheduler.hxx"
#include "cosmos/proc/ChildCloner.hxx"
#include "cosmos/proc/SigSet.hxx"
#include "cosmos/proc/SubProc.hxx"
#include "cosmos/proc/clone.hxx"
#include "cosmos/proc/process.hxx"
#include "cosmos/utils.hxx"

namespace cosmos {

namespace {

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
	//
	// Another alternative would be using regular fork() and create a
	// PidFD from the child PID. As long as no one is collecting the child
	// status via any of the wait() functions this would also be race
	// free.
	PidFD pidfd;
	CloneArgs clone_args;
	clone_args.setPidFD(pidfd);
	clone_args.setFlags({CloneFlag::CLEAR_SIGHAND, CloneFlag::PIDFD});

	if (auto pid = proc::clone(clone_args); pid != std::nullopt) {
		// parent process with child pid
		return SubProc{*pid, pidfd};
	}

	// the child process -- let's do something!

	try {
		postFork();

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
