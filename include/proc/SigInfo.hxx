#pragma once

// C++
#include <optional>

// Linux
// this contains SYS_SECCOMP but the header has conflicts with user space
// headers for some time already, there seems to be no way around it:
//     https://bugzilla.kernel.org/show_bug.cgi?id=200081
// We define the constant ourselves further below.
//#include <linux/signal.h>

// cosmos
#include <cosmos/error/errno.hxx>
#include <cosmos/fs/types.hxx>
#include <cosmos/io/types.hxx>
#include <cosmos/memory.hxx>
#include <cosmos/proc/ptrace.hxx>
#include <cosmos/proc/signal.hxx>
#include <cosmos/proc/types.hxx>
#include <cosmos/time/types.hxx>
#include <cosmos/utils.hxx>

#ifndef SYS_SECCOMP
#	define SYS_SECCOMP 1
#endif
#ifndef SYS_USER_DISPATCH
#	define SYS_USER_DISPATCH 2
#endif

/**
 * @file
 *
 * Wrapper around the siginfo_t type.
 *
 * Providing a type safe, clear and efficient API for siginfo_t is a difficult
 * task. The struct is big (~150 bytes), contains some union fields, many
 * fields are only valid in specific contexts and some fields have conflicting
 * meanings depending on context.
 *
 * The structure is typically only filled in by the kernel or glibc, not by
 * applications. It is possible to use it in custom ways via the low level
 * system call `rt_sigqueueinfo()`, though. The latter is also problematic in
 * terms of trusting the siginfo_t data received from the kernel. If the
 * signal source is another userspace process then the structure could contain
 * rather arbitrary data, breaking the interface contract as documented in the
 * sigaction(2) man page.
 *
 * The libcosmos API focuses on interpreting data from siginfo_t received from
 * the kernel and conforming to the API contract. For special use cases
 * applications can access the raw data structure.
 *
 * Depending on the signal number and signal source different interfaces need
 * to be provided to siginfo_t. We wouldn't want to copy the full ~150 bytes
 * all the time just for accessing the data using the proper types. This can
 * be addressed by either always keeping siginfo_t on the heap and using a
 * shared_ptr to manage the lifetime of all representations of the data. This
 * limits the way in which siginfo_t related APIs can be used, though.
 * Application might not want to involve the heap for this. An alternative is
 * to provide types that only carry the relevant data, not the full siginfo_t
 * anymore. This way not the fully blown data structure needs to be copied and
 * the overhead remains low, since most signal contexts only use a few fields
 * from siginfo_t. This is what libcosmos currently does.
 *
 * For getting a better idea of the union structure you can have a look at the
 * `union __sifields` in the system headers.
 **/

namespace cosmos {

/// Signal information struct used when receiving signals.
/**
 * This kernel data structure is union-like and most of its fields only have
 * meaning - and sometimes different meanings - depending on the
 * SigInfo::Source value. Thus most information is returned as a std::optional
 * containing a separate SigInfo::<Kind>Data type which contains the
 * specialized data relevant for the context.
 *
 * TODO: check whether it makes sense to add TrapData for SIGTRAP. It is a bit
 * unclear what exactly to expect, since ptrace(2) is insanely complex.
 **/
class COSMOS_API SigInfo {
public: // types

	/// The source of a signal.
	enum class Source : int {
		USER    = SI_USER,    ///< sent via kill().
		KERNEL  = SI_KERNEL,  ///< sent by the kernel.
		QUEUE   = SI_QUEUE,   ///< sent from user space via sigqueue().
		TIMER   = SI_TIMER,   ///< POSIX timer expired.
		MESGQ   = SI_MESGQ,   ///< POSIX message queue state changed.
		ASYNCIO = SI_ASYNCIO, ///< AIO completed.
		QSIGIO  = SI_SIGIO,   ///< queued SIGIO (only up to Linux 2.2).
		TKILL   = SI_TKILL,   ///< sent by tkill() or tgkill().
	};

	/// Information about the process a signal is from or about.
	/**
	 * Note that the pid and uid information is not necessarily to be
	 * trusted, `rt_sigqueueinfo()` allows user space to fill in arbitrary
	 * values here. Only privileged processes or processes running under
	 * the same UID as the target process may send signals. In some
	 * scenarios this may still be an issue.
	 *
	 * For Source::KERNEL the values should be safe, though. See also
	 * isTrustedSource().
	 **/
	struct ProcessCtx{
		ProcessID pid; ///< PID of the process
		UserID uid;    ///< real user ID of the process
	};

	/// Additional custom SigInfo data.
	/**
	 * Some SigInfo contexts allow to add custom data either as an `int`
	 * or a `void*`. The meaning and format of this data is application
	 * specific, so you need to known by contract what to expect.
	 **/
	struct CustomData {
	public: // functions

		explicit CustomData(union sigval val) : m_val{val} {}

		/// Returns custom data sent with the signal.
		/**
		 * The data can either be provided as an `int` or as a
		 * `void*`. The latter is available from asPtr().
		 **/
		int asInt() const {
			return m_val.sival_int;
		}

		/// Returns custom data sent with the signal.
		/**
		 * This returns custom data sent with the signal, placed into a
		 * `void*` field.
		 *
		 * \see asInt()
		 **/
		void* asPtr() const {
			return m_val.sival_ptr;
		}

	protected: // data

		union sigval m_val;
	};

	/// Additional data found in SigInfo with Source::USER.
	struct UserSigData {
		/// The PID and real user ID of the sending process.
		ProcessCtx sender;
	};

	/// Additional data found in SigInfo with Source::QUEUE.
	struct QueueSigData {
		/// The PID and real user ID of the sending process.
		ProcessCtx sender;

		/// Custom data supplied along with the signal.
		CustomData data;
	};

	/// Additional data found in SigInfo with Source::MESGQ.
	struct MsgQueueData {
		/// The PID and real user ID of the process that sent a message queue message.
		ProcessCtx msg_sender;

		/// Custom data supplied via `mq_notify()`.
		CustomData data;
	};

	/*
	 * TODO: this contains CustomData which is actually the ID returned from timer_create().
	 * We need to model types for `timer_create()` first to make use of
	 * that.
	 */
	/// Additional data found in SigInfo with Source::TIMER.
	struct TimerData {
	public: // types

		enum class TimerID : int {};

	public: // functions

		/// The ID of the timer which expired.
		/**
		 * This field is a Linux extension. This ID is not the same as the ID
		 * returned from `timer_create()`, therefore it is a distinct type
		 * defined for this purpose only.
		 **/
		TimerID id;

		/// The timer overrun count.
		/**
		 * This field is a Linux extension. It is equal to the information
		 * obtained from `timer_getoverrun()`.
		 **/
		int overrun;
	};

	/// Additional data found in SigInfo for one of the memory fault / trap signals.
	/**
	 * The data for fault type signals is quite complex. There are some
	 * fields only available on certain architectures like IA64, Alpha and
	 * Sparc (all discontinued architectures). These are not currently
	 * covered here.
	 *
	 * This is only a base type for more concrete fault data like IllData
	 * delivered with SIGILL.
	 **/
	struct FaultData {

		/// The address of the fault / trap.
		void *addr;
	};

	/// Additional data delivered with SIGILL signals.
	struct IllData : public FaultData {
	public: // types

		/// Different reasons for delivering a SIGILL signal.
		enum class Reason : int {
			OPCODE    = ILL_ILLOPC,   ///< illegal opcode.
			OPERAND   = ILL_ILLOPN,   ///< illegal operand.
			ADDRESS   = ILL_ILLADR,   ///< illegal addressing mode.
			TRAP      = ILL_ILLTRP,   ///< illegal trap.
			PRIV_OP   = ILL_PRVOPC,   ///< privileged opcode.
			PRIV_REG  = ILL_PRVREG,   ///< privileged register.
			COPROC    = ILL_COPROC,   ///< coprocessor error.
			BAD_STACK = ILL_BADSTK,   ///< internal stack error.
			BAD_IADDR = ILL_BADIADDR, ///< unimplemented instruction address.
		};

	public: // data

		/// The reason why SIGILL was delivered.
		Reason reason;
	};

	/// Extra data delivered with SIGFPE signals.
	struct FPEData : public FaultData {
	public: // types

		/// Different reasons for delivering floating-point exceptions.
		enum class Reason : int {
			INT_DIV_ZERO    = FPE_INTDIV, ///< integer divide by zero.
			INT_OVERFLOW    = FPE_INTOVF, ///< integer overflow.
			FLOAT_DIV_ZERO  = FPE_FLTDIV, ///< floating-point divide by zero.
			FLOAT_OVERFLOW  = FPE_FLTOVF, ///< floating-point overflow.
			FLOAT_UNDERFLOW = FPE_FLTUND, ///< floating-point underflow.
			FLOAT_INEXACT   = FPE_FLTRES, ///< floating-point inexact result.
			FLOAT_INVALID   = FPE_FLTINV, ///< floating-point invalid operation.
			FLOAT_SUB_RANGE = FPE_FLTSUB, ///< subscript out of range.
			FLOAT_UNKNOWN   = FPE_FLTUNK, ///< undiagnosed floating-point exception.
			FLOAT_CONDTRAP  = FPE_CONDTRAP, ///< trap on condition.
		};

	public: // data

		/// The reason why SIGFPE was delivered.
		Reason reason;
	};

	/// Additional data delivered with SIGSEGV signals.
	struct SegfaultData : public FaultData {
	public: // types

		/// Different reasons for delivering a SIGSEGV signal.
		enum class Reason : int {
			MAP_ERROR      = SEGV_MAPERR, ///< address not mapped to an object.
			ACCESS_ERROR   = SEGV_ACCERR, ///< invalid permissions for mapped object.
			BOUND_ERROR    = SEGV_BNDERR, ///< failed address bound checks.
			PROT_KEY_ERROR = SEGV_PKUERR, ///< access was denied by memory protection keys (pkeys(7)).
			ACCESS_ADI     = SEGV_ACCADI, ///< ADI (application data integrity) not enabled for mapped object (SPARC specific).
			MCD_DISRUPT    = SEGV_ADIDERR, ///< disrupting MCD error (Sparc ADI specific).
			PRECISE_MCD    = SEGV_ADIPERR, ///< precise MCD exception (Sparc ADI specific).
			ASYNC_MTE      = SEGV_MTEAERR, ///< asynchronous ARM MTE error.
			SYNC_MTE       = SEGV_MTESERR, ///< synchronous ARM MTE error.
			CPROT_ERROR    = SEGV_CPERR,   ///< control protection error (aarch64 guarded control stack).
		};

		struct Bound {
			void *lower = nullptr;
			void *upper = nullptr;
		};

		// TODO: this is preliminary strong type that should be moved into a pkey() API wrapper.
		enum class ProtectionKey : unsigned int {
		};

	public: // data

		/// The reason why SIGSEGV was delivered.
		Reason reason;
		/// For Reason::BOUND_ERROR this contains the lower and upper bound.
		std::optional<Bound> bound;
		/// For Reason::PROT_KEY_ERROR this contains the protection key that caused the fault.
		std::optional<ProtectionKey> key;
	};

	/// Additional data delivered with SIGBUS signals.
	struct BusData : public FaultData {
	public: // types

		/// Different reasons for delivering a SIGBUS signal.
		enum class Reason : int {
			ALIGNMENT           = BUS_ADRALN,   ///< invalid address alignment.
			NOT_EXISTING        = BUS_ADRERR,   ///< nonexistent physical address.
			OBJECT_ERROR        = BUS_OBJERR,   ///< Object-specific hardware error.
			MCE_ACTION_REQUIRED = BUS_MCEERR_AR, ///< hardware memory error consumed on a machine check; action required.
			MCE_ACTION_OPTIONAL = BUS_MCEERR_AO, ///< hardware memory error detected in process but not consumed; action optional.
		};

	public: // data

		/// The reason why SIGBUS was delivered.
		Reason reason;
		/// For Reason::MCE_ACTION_REQUIRED and Reason::MCE_ACTION_OPTIONAL this contains the least significant bit of the reported address.
		std::optional<short> addr_lsb;
	};

	/// Additional data found in SigInfo with SIGCHILD.
	struct ChildData {
	public: // types

		/// Types of SIGCHLD events that can occur.
		enum class Event : int {
			INVALID   = -1,
			EXITED    = CLD_EXITED,   ///< Child has exited.
			KILLED    = CLD_KILLED,   ///< Child was killed.
			DUMPED    = CLD_DUMPED,   ///< Child terminated abnormally due to a signal, dumping core.
			TRAPPED   = CLD_TRAPPED,  ///< Traced child has trapped.
			STOPPED   = CLD_STOPPED,  ///< Child has stopped due to a signal.
			CONTINUED = CLD_CONTINUED ///< Stopped child has continued.
		};

	public: // functions

		/// Returns whether the child exited.
		bool exited() const { return event == Event::EXITED; }

		/// Returns whether the child was killed by a signal.
		bool killed() const { return event == Event::KILLED; }

		/// Returns whether the child dumped core due to a signal.
		bool dumped() const { return event == Event::DUMPED; }

		/// Returns true if the child entered a tracing trap.
		bool trapped() const { return event == Event::TRAPPED; }

		/// Returns whether the child continued due to a signal.
		bool continued() const { return event == Event::CONTINUED; }

		/// Returns whether the child stopped.
		bool stopped() const { return event == Event::STOPPED; }

		/// Returns whether the child exited and had an exit status of 0.
		bool exitedSuccessfully() const {
			return exited() && *status == ExitStatus::SUCCESS;
		}

		/// Returns whether the child received a signal.
		bool signaled() const {
			return event == Event::KILLED ||
				event == Event::DUMPED ||
				event == Event::STOPPED ||
				event == Event::CONTINUED;
		}

		/// Returns whether the structure contains valid information.
		bool valid() const {
			return event != Event::INVALID;
		}

		void reset() {
			event = Event::INVALID;
			child.pid = ProcessID::INVALID;
			status = std::nullopt;
			signal = std::nullopt;
		}

	public: // data

		/// The kind of child process event that occurred.
		Event event;
		/// the PID and its real user ID the signal is about.
		ProcessCtx child;

		/// Contains the process's exit status, if applicable.
		/**
		 * An exit status is only available for Event::EXITED. In the other
		 * cases a `signal` is available instead.
		 **/
		std::optional<ExitStatus> status;

		/// Contains the signal number that caused the child process to change state.
		/**
		 * This signal number is only available for events other than
		 * Event::EXITED. Otherwise `status` is available instead.
		 **/
		std::optional<Signal> signal;

		/// The CPU time the child spent in user space.
		/**
		 * This does not include the time of waited-for children of
		 * the child.
		 *
		 * This data is not available from the cosmos::proc::wait()
		 * family of functions.
		 **/
		std::optional<ClockTicks> user_time;

		/// The CPU time the child spent in kernel space.
		/**
		 * This does not include the time of waited-for children of the child.
		 *
		 * This data is not available from the cosmos::proc::wait()
		 * family of functions.
		 **/
		std::optional<ClockTicks> system_time;
	};

	/// Additional data found in SigInfo delivered with SIGSYS.
	struct SysData {
	public: // types

		/// Different reasons for delivering SIGYS.
		enum class Reason : int {
			SECCOMP       = SYS_SECCOMP, ///< triggered by a seccomp(2) filter rule, SECCOMP_RET_TRAP.
			USER_DISPATCH = SYS_USER_DISPATCH, ///< triggered by syscall user dispatch.
		};

	public: // data

		/// Why SIGSYS was delivered.
		Reason reason;
		/// The calling user space instruction.
		void *call_addr;
		/// The system call number.
		int call_nr;
		/// The system call ABI.
		ptrace::Arch arch;
		/// The SECCOMP_RET_DATA portion or Errno::SUCCESS if seccomp is not involved.
		Errno error;
	};

	/// Additional data found in SigInfo with SIGPOLL.
	struct PollData {
	public: // types

		/// Different reasons for delivering SIGPOLL.
		enum class Reason : int {
			INPUT    = POLL_IN,  ///< Data input available.
			OUTPUT   = POLL_OUT, ///< Output buffers available.
			MESSAGE  = POLL_MSG, ///< Input message available.
			ERROR    = POLL_ERR, ///< I/O error.
			PRIORITY = POLL_PRI, ///< High priority input available.
			HANGUP   = POLL_HUP, ///< Device disconnected.
		};

	public: // data

		Reason reason;
		/// The file descriptor for which the event occurred.
		FileNum fd;
		/// The I/O events that occurred for `fd`.
		PollEvents events;
	};

public: // functions

	/// Creates a zero-initialized SigInfo wrapper.
	SigInfo() {
		clear();
	}

	/// Leaves the underlying data structure uninitialized.
	/**
	 * When SigInfo is used as an output parameter only (the typical case)
	 * then you can invoke this constructor to avoid unnecessary
	 * zero-initialization.
	 **/
	SigInfo(const no_init_t) {}

	/// Returns the signal number that occurred.
	Signal sigNr() const {
		return Signal{SignalNr{m_raw.si_signo}};
	}

	/// Returns the source of the signal.
	/**
	 * For some special signals SigSource::KERNEL is implied if
	 * isTrustedSource() returns `true`. These special signals are
	 * SIGFPE, SIGBUS, SIGILL, SIGSEGV, SIGBUS, SIGTRAP, SIGCHILD,
	 * SIGPOLL/SIGIO and SIGSYS.
	 *
	 * These signals use the `si_code` field for special data, but their
	 * source is the kernel, if isTrustedSource() returns `true`.
	 * Implying SigSource::KERNEL in these situations allows us to always
	 * return a value here instead of a std::optional, which could be
	 * empty in these cases.
	 *
	 * Since other user space processes are allowed to set arbitrary
	 * Source values smaller than 0 it can happen that values outside the
	 * defined Source constants are returned here. The interpretation of
	 * source() is application specific in these cases (or should be
	 * ignored, if not expected).
	 **/
	Source source() const;

	/// Returns whether the signal was sent from a trusted source (i.e. the kernel).
	/**
	 * Only the kernel is allowed to set an `si_code` >= 0. This is an
	 * indicator whether we can fully trust the integrity of the data
	 * contained in the siginfo_t.
	 *
	 * An exception is when a process sends itself a signal, but this can
	 * also be considered a trusted source in all but very special cases
	 * (like executing untrusted code in another thread?).
	 **/
	bool isTrustedSource() const {
		return m_raw.si_code >= 0;
	}

	/// Returns whether the signal is one of the fault signals.
	bool isFaultSignal() const;

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
	 * This data is only available for `sigNr() == signal::BAD_SYS`. This
	 * signal is used for seccomp mainly and in some situations when the
	 * kernel deems it necessary (not simply if a bad system call nr. is
	 * passed).
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

	/// Returns SIGILL specific data.
	/**
	 * This data is only available if `sigNr() == signal::ILL`.
	 **/
	std::optional<const IllData> illData() const;

	/// Returns SIGFPE specific data.
	/**
	 * This data is only available if `sigNr() == signal::FPE`.
	 **/
	std::optional<const FPEData> fpeData() const;

	/// Returns SIGSEGV specific data.
	/**
	 * This data is only available if `sigNr() == signal::SEGV`.
	 **/
	std::optional<const SegfaultData> segfaultData() const;

	/// Returns SIGBUS specific data.
	/**
	 * This data is only available if `sigNr() == signal::BUS`.
	 **/
	std::optional<const BusData> busData() const;

	/// Zeroes out the low level siginfo_t data structure.
	void clear() {
		zero_object(m_raw);
	}

	const siginfo_t* raw() const {
		return &m_raw;
	}

	siginfo_t* raw() {
		return &m_raw;
	}

protected: // functions

	/// Returns an error code that is generally unused on Linux (always 0).
	/**
	 * An exception is the case of SIGSYS generated by seccomp(2) filters.
	 **/
	Errno error() const {
		return Errno{m_raw.si_errno};
	}

	ProcessCtx procCtx() const {
		return ProcessCtx{pid(), uid()};
	}

	ProcessID pid() const {
		return ProcessID{m_raw.si_pid};
	}

	UserID uid() const {
		return UserID{m_raw.si_uid};
	}

protected: // data

	siginfo_t m_raw;
};

} // end ns
