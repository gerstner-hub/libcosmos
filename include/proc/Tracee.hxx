#pragma once

// C++
#include <optional>
#include <vector>

// cosmos
#include <cosmos/compiler.hxx>
#include <cosmos/io/iovector.hxx>
#include <cosmos/proc/process.hxx>
#include <cosmos/proc/ptrace.hxx>
#include <cosmos/proc/signal.hxx>
#include <cosmos/proc/SigSet.hxx>
#include <cosmos/utils.hxx>

// forward declaration of low-level structures found in sys/user.h.
// avoid pulling in that low level header here, since this way of reading
// registers is not common anymore.
struct user_regs_struct;
struct user_fpregs_struct;
// seccomp instruction
struct sock_filter;

#ifdef COSMOS_X86
struct user_desc;
#endif

namespace cosmos {

struct InputMemoryRegion;
class SigInfo;

/// Thin wrapper class around the `ptrace()` system call.
/**
 * This is a type safe wrapper around the `ptrace()` system call. An instance
 * of this type always operates on the same process supplied during
 * construction time. There are no resources managed by this class.
 *
 * The ptrace() API is highly complex and this class can only offer some basic
 * wrappers and documentation about it.
 **/
class COSMOS_API Tracee {
public: // types

	/// Different ways to restart a tracee.
	/**
	 * This is a sub-set of the commands from ptrace::Request that deal
	 * with restarting the tracee in different ways. All of these requests
	 * optionally accept a signal to inject, except for
	 * RestartMode::LISTEN.
	 **/
	enum class RestartMode {
		CONT              = to_integral(ptrace::Request::CONT),
		DETACH            = to_integral(ptrace::Request::DETACH),
		SYSCALL           = to_integral(ptrace::Request::SYSCALL),
		SINGLESTEP        = to_integral(ptrace::Request::SINGLESTEP),
#ifdef PTRACE_SYSEMU
		SYSEMU            = to_integral(ptrace::Request::SYSEMU),
		SYSEMU_SINGLESTEP = to_integral(ptrace::Request::SYSEMU_SINGLESTEP),
#endif
		LISTEN            = to_integral(ptrace::Request::LISTEN)
	};

public: // functions

	explicit Tracee(const ProcessID pid = ProcessID::INVALID) :
			m_pid{pid}
	{}

	ProcessID pid() const {
		return m_pid;
	}

	bool valid() const {
		return pid() != ProcessID::INVALID;
	}

	/// Seize a tracee.
	/**
	 * This is the modern way of making a process a tracee. This does not
	 * stop the process. The seize property is inherited to matching child
	 * processes of the tracee if one of the options
	 * Opt::TRACEFORK, Opt::TRACEVFORK or Opt::TRACECLONE is set on the
	 * tracee.
	 *
	 * Initial tracing options are set atomically alongside the SEIZE
	 * request.
	 **/
	void seize(const ptrace::Opts opts) {
		this->request(ptrace::Request::SEIZE, nullptr, opts.raw());
	}

	/// Attach to a process, making it a tracee.
	/**
	 * This is the old method of making a process a tracee. Tracers
	 * attached to this way don't support all ptrace() operations and it
	 * is not recommended to use this method anymore.
	 *
	 * The tracee will be sent a SIGSTOP signal, the tracer needs to wait
	 * on the tracee to assert it has entered stop state as a result of
	 * the attach operation. The synthetic SIGSTOP event should be
	 * suppressed by the tracer.
	 *
	 * This method of attaching to the tracee has inherent race
	 * conditions. Other signals can concurrently occur while the tracer
	 * attempts to attach to it. Events other than SIGSTOP should be
	 * reinjected until SIGSTOP is observed. This does not reliably work
	 * if SIGSTOP itself is occurring in parallel, making attach()
	 * unreliable.
	 **/
	void attach() {
		this->request(ptrace::Request::ATTACH);
	}

	/// Detach from and restart the tracee.
	/**
	 * The tracee will be restarted (if currently in a tracing stop), the
	 * process will no longer be traced. This method can be used
	 * regardless of whether the tracee was seize()'d or attach()'d to.
	 **/
	void detach() {
		this->request(ptrace::Request::DETACH);
	}

	/// Continues a traced process, optionally delivering `signal`.
	/**
	 * If the current ptrace stop state doesn't allow injection of a signal, then
	 * none should be specified. Generally only a signal-stop state allows
	 * injection of signals.
	 *
	 * If signal information has been overwritten by using ptrace::set_siginfo(),
	 * then the `signal` passed here must match, otherwise the behaviour will be
	 * undefined.
	 **/
	void restart(const RestartMode mode, const std::optional<Signal> signal = {}) {
		// data here takes the plain signal number, 0 means "don't inject a signal".
		this->request(static_cast<ptrace::Request>(mode),
				nullptr,
				mode != RestartMode::LISTEN && signal ? signal->raw() : SignalNr{0});
	}

	/// Interrupt the tracee.
	/**
	 * This works only on tracees attached via seize(). As a result to the
	 * interrupt the tracee can enter:
	 *
	 * - syscall-exit-stop if ptrace::Request::SYSCALL is in effect. The
	 *   interrupted system call is restarted when the tracee is
	 *   restarted.
	 * - if the tracee was already stopped by a signal and
	 *   ptrace::Request::LISTEN was sent to it then a ptrace-event-stop
	 *   is reported with the stop signal.
	 * - if another ptrace-stop is triggered in parallel, then this stop
	 *   happens.
	 * - otherwise a ptrace-event-stop for signal SIGTRAP happens.
	 **/
	void interrupt() {
		this->request(ptrace::Request::INTERRUPT);
	}

	/// Set tracing options for the given tracee.
	/**
	 * This call completely defines the options in effect for the given
	 * tracee. These options can be inherited by new tracees that are
	 * auto-attached via the TRACEFORK, TRACEVFORK and TRACECLONE options.
	 **/
	void setOptions(const ptrace::Opts opts) {
		this->request(ptrace::Request::SETOPTIONS, nullptr, opts.raw());
	}

	/// Read one word of data from the tracee's memory.
	/**
	 * `addr` specifies the address in the tracee's memory to read a word
	 * from.
	 *
	 * The size of the word is defined by the type of operating system and
	 * architecture of the system. On Linux no differentiation between
	 * TEXT and DATA is made, thus only DATA is offered here.
	 **/
	long peekData(const long *addr) const {
		return *(this->request(ptrace::Request::PEEKDATA, addr));
	}

	/// Write one word of data into the tracee's memory.
	void pokeData(const long *addr, long value) {
		this->request(ptrace::Request::POKEDATA, addr, value);
	}

	/// Read one word of data from the tracee's user area.
	/**
	 * The user area refers to the kernel's `struct user` which contains
	 * data about registers and other information about the process.
	 * This data is highly OS and architecture specific and could yield
	 * unexpected results.
	 *
	 * The given parameter is an `offset` into `struct user` where to read
	 * from. The offset typically needs to be word-aligned.
	 **/
	long peekUser(const long *offset) const {
		return *(this->request(ptrace::Request::PEEKUSER, offset));
	}

	/// Change one word of data in the tracee's user area.
	/**
	 * This changes on word of data in the tracee's user area. \see
	 * peekUser(). `offset` typically needs to be word-aligned.
	 **/
	void pokeUser(const long *offset, long value) {
		this->request(ptrace::Request::POKEUSER, offset, value);
	}

#ifdef PTRACE_GETREGS
	/// Copy the tracee's general purpose registers into the provided structure.
	/**
	 * You need to include sys/user.h and check out the data structure
	 * found in there for details. This is a low level structure specially
	 * designed for GDB and also not available on all architectures.
	 *
	 * Preferably use getRegisterSet() instead.
	 **/
	void getRegisters(struct user_regs_struct &out) const {
		// NOTE: on Sparc the address needs to be passed as third
		// argument, not as fourth. If we should ever want to support
		// it, we'd need an #ifdef of some sort.
		this->request(ptrace::Request::GETREGS, nullptr, &out);
	}
#endif

#ifdef PTRACE_SETREGS
	/// Modify the tracee's general purpose registers.
	/**
	 * Some register modifications may be disallowed by the kernel to
	 * maintain integrity of the tracee.
	 **/
	void setRegisters(const struct user_regs_struct &out) {
		// NOTE: see getRegisters() about Sparc architecture
		this->request(ptrace::Request::SETREGS, nullptr, &out);
	}
#endif

#ifdef PTRACE_GETFPREGS
	/// Copy the tracee's floating point registers into the provided structure.
	/**
	 * This is similar to getRegisters() but provides the floating point
	 * registers instead.
	 *
	 * Preferably use getRegisterSet() instead.
	 **/
	void getFloatRegisters(struct user_fpregs_struct &out) const {
		// NOTE: see getRegisters() about Sparc architecture
		this->request(ptrace::Request::GETFPREGS, nullptr, &out);
	}
#endif

#ifdef PTRACE_SETFPREGS
	/// Modify the tracee's floating point registers.
	/**
	 * \see setRegisters().
	 **/
	void setFloatRegisters(const struct user_fpregs_struct &out) {
		this->request(ptrace::Request::SETFPREGS, nullptr, &out);
	}
#endif

	/// Retrieve a set of registers from the tracee.
	/**
	 * This retrieves binary data based on an I/O vector. For
	 * ptrace::RegisterType::GENERAL_PURPOSE the target data structure is
	 * found in elf.h called `elf_gregset_t`.
	 *
	 * The kernel will update `iovec` to reflect the actual amount of data
	 * that has been returned.
	 **/
	void getRegisterSet(const ptrace::RegisterType type, InputMemoryRegion &iovec) const {
		this->request(ptrace::Request::GETREGSET, type, iovec.asIovec());
	}

	/// Modify a set of registers in the tracee.
	void setRegisterSet(const ptrace::RegisterType type, OutputMemoryRegion &iovec) {
		this->request(ptrace::Request::SETREGSET, type, iovec.asIovec());
	}

	/// Obtain information about the signal that caused the stop.
	void getSigInfo(SigInfo &info) const;

	/// Set signal information for the tracee.
	/**
	 * This will affect only signals that would normally be delivered to
	 * the tracee and were caught by the tracer. These signals can be hard
	 * to tell from synthetic signals generated by ptrace() itself.
	 *
	 * When changing the signal information this way then the signal
	 * passed to restart() needs to match, to prevent undefined behaviour.
	 **/
	void setSigInfo(const SigInfo &info);

	/// Obtains SigInfo structures pending for the tracee.
	/**
	 * Based on `settings` obtain a numer of SigInfo structures pending
	 * for the tracee. `settings` define how many SigInfo will be
	 * retrieved at max and from what position in the signal queue.
	 *
	 * There is no way to know how many entries exist currently (this
	 * information can also rapidly change). If no more SigInfo structures
	 * exist at the given position then a short or zero item count is
	 * returned.
	 **/
	std::vector<SigInfo> peekSigInfo(const ptrace::PeekSigInfo &settings);

	/// Obtain the tracee's mask of blocked signals.
	void getSigMask(SigSet &set) const {
		this->request(ptrace::Request::GETSIGMASK, sizeof(*set.raw()), set.raw());
	}

	/// Change the tracee's mask of blocked signals.
	void setSigMask(const SigSet &set) {
		this->request(ptrace::Request::SETSIGMASK, sizeof(*set.raw()), set.raw());
	}

	/// Returns the PID of a newly created child of the tracee in the context of a ptrace-event-stop.
	/**
	 * This call is only valid during a ptrace-event-stop and when
	 * ptrace::Event::FORK, ptrace::EVENT::VFORK,
	 * ptrace::Event::VFORK_DONE or ptrace::Event::CLONE is reported.
	 *
	 * The return value is the PID of the newly created child process.
	 **/
	ProcessID getPIDEventMsg() const {
		const auto pid = getEventMsg();
		return static_cast<ProcessID>(pid);
	}

	/// Returns the tracee's WaitStatus in of a ptrace-event-stop.
	/**
	 * This call is only valid during a ptrace-event-stop when
	 * ptrace::Event::EXIT is reported.
	 *
	 * The return value either contains the ExitStatus in case of a
	 * regular exit or the Signal by which the process was killed.
	 **/
	WaitStatus getExitEventMsg() const {
		const auto status = getEventMsg();
		return WaitStatus{static_cast<int>(status)};
	}

	/// Returns the SECCOMP_RET_DATA in the context of a ptrace-event-stop.
	/**
	 * This request is only valid during a ptrace-event-stop when
	 * ptrace::Event::SECCOMP is reported.
	 *
	 * The return value is the 16-bit value known as SECCOMP_RET_DATA, see
	 * `seccomp(2)`.
	 **/
	uint16_t getSeccompRetDataEventMsg() const {
		const auto seccomp_ret_data = getEventMsg();
		return static_cast<uint16_t>(seccomp_ret_data);
	}

	/// Retrieve a classic seccomp BPF program installed in the tracee.
	/**
	 * `prog_index` is the index of the program to return, where index 0
	 * is the most recently installed program. If the index is greater
	 * than the number of installed programs then an ApiError with
	 * Errno::NO_ENTRY is thrown.
	 *
	 * If `instructions` is empty then the call will first ask the kernel
	 * how big the given program is, to dimension `instructions`
	 * accordingly. In a second call the program is retrieved into the
	 * vector.
	 *
	 * If `instructions` is non-empty then the provided size will be used.
	 * Note that there seems to be error handling missing in the kernel to
	 * detect when the provided vector is too small. This means a too
	 * small vector could lead to memory corruption in the process.
	 **/
	void getSeccompFilter(std::vector<struct sock_filter> &instructions, const unsigned long prog_index) const;

#ifdef COSMOS_X86

	// these operations are also supported on MIPS, m68k and C-SKY but
	// they're using different data structures.
	// for the definition of `struct user_desc` you need to include <asm/ldt.h>.

	/// Retrieve a TLS entry from the tracee's GDT.
	/**
	 * The entry number provided in `desc.entry_number` will be retrieved
	 * and stored into `desc`.
	 **/
	void getThreadArea(struct user_desc &desc) const;

	/// Change a TLS entry in the tracee's GDT.
	/**
	 * This call cannot be used to allocated new TLS entries. It can only
	 * be used to overwrite existing ones.
	 **/
	void setThreadArea(const struct user_desc &desc);

#endif // X86

	/// Returns system call information in the context of the current ptrace stop.
	/**
	 * This request is only valid during syscall-entry-stop,
	 * syscall-exit-stop or ptrace-event-stop for ptrace::Event::SECCOMP.
	 *
	 * Depending on the type of stop that occurred `info` will contain
	 * different data, thus only certain parts of the struct are
	 * accessible via std::optional return values.
	 **/
	void getSyscallInfo(ptrace::SyscallInfo &info) const;

protected: // functions

	/// Returns the current event message for a ptrace-event-stop.
	/**
	 * The interpretation of the returned value depends on the
	 * ptrace::Event that has been reported. If there is no (matching)
	 * ptrace-event, then the return value seems to be undefined.
	 **/
	unsigned long getEventMsg() const {
		unsigned long msg = 0;
		this->request(ptrace::Request::GETEVENTMSG, nullptr, &msg);
		return msg;
	}

	template <typename ADDR=void*, typename DATA=void*>
	std::optional<long> request(const ptrace::Request req, ADDR addr = nullptr, DATA data = nullptr) const {
		// First cast to const void* then remove const, this allows to
		// pass in const pointers. The compiler cannot understand that
		// ptrace() won't change the pointed-to data according to
		// contract.
		return cosmos::ptrace::trace(req, m_pid,
				const_cast<void*>(reinterpret_cast<const void*>(addr)),
				const_cast<void*>(reinterpret_cast<const void*>(data)));
	}

	/*
	 * Template specialization in case `nullptr` is passed for addr.
	 * This avoids invalid cast from nullptr_t to void* errors.
	 */
	template <typename DATA=void*>
	std::optional<long> request(const ptrace::Request req, std::nullptr_t, DATA data = nullptr) const {
		return request(req, static_cast<void*>(nullptr), data);
	}

protected: // data

	ProcessID m_pid;
};

} // end ns
