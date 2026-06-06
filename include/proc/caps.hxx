#pragma once

// Linux
#include <sys/capability.h>

namespace cosmos {

/// Different Linux capabilities supported by the kernel.
/**
 * Capabilities offer a more fine-grained permissions model over the
 * traditional root/non-root approach. Be aware that many capabilities still
 * easily allow to escalate to full root privileges.
 *
 * Capabilities follow rather complex rules. Different capability sets exist
 * which define what kind of privilege transitions are possible. Traditionally
 * they could only be set on files as extended attributes in a fashion similar
 * to setuid-root binaries. These days also "ambient" capabilities exist which
 * can be transparently inherited to child processes.
 **/
enum class Capability : long {
	/// Change file user and group ownership.
	CHOWN            = CAP_CHOWN,
	/// Access arbitrary files (override discretionary access controls).
	DAC_OVERRIDE     = CAP_DAC_OVERRIDE,
	/// Read and search access to arbitrary files (override discretionary access controls).
	DAC_READ_SEARCH  = CAP_DAC_READ_SEARCH,
	/// Ignore file ownership checks in access controls.
	FOWNER           = CAP_FOWNER,
	/// Don't clear setuid/setgid bit when a file is modified.
	FSETID           = CAP_FSETID,
	/// Send signals to arbitrary processes.
	KILL             = CAP_KILL,
	/// Set arbitrary group IDs.
	/**
	 * Allow to set arbitrary GIDs as process credentials via setgid(2),
	 * setgroups(2) or when passing credentials on a socket.
	 **/
	SETGID           = CAP_SETGID,
	/// Set arbitrary user IDs.
	/**
	 * Analogous got CAP_SETGID for user IDs.
	 **/
	SETUID           = CAP_SETUID,
	/// Perform arbitrary modifications on a process's capability sets.
	/**
	 * - transfer any capability from the bounding set to the inheritable
	 *   set.
	 * - take bits out of the bounding set.
	 * - modify the securebits of the process.
	 **/
	SETPCAP          = CAP_SETPCAP,
	/// Allow modification of the S_IMMUTABLE and S_APPEND file attrs.
	IMMUTABLE        = CAP_LINUX_IMMUTABLE,
	/// Allow to bind IP ports below 1024 or ATM VCIs below 12.
	NET_BIND_SERVICE = CAP_NET_BIND_SERVICE,
	/// Allow network broadcasting and listening to multicast.
	NET_BROADCAST    = CAP_NET_BROADCAST,
	/// Allow all kinds network administration changes.
	NET_ADMIN        = CAP_NET_ADMIN,
	/// Allow the use of RAW and PACKET sockets as well as binding to any address.
	NET_RAW          = CAP_NET_RAW,
	/// Allow `mlock()` and `mlockall()` on arbitrary memory.
	IPC_LOCK         = CAP_IPC_LOCK,
	/// Override IPC ownership checks.
	IPC_OWNER        = CAP_IPC_OWNER,
	/// Insert and remove kernel modules.
	SYS_MODULE       = CAP_SYS_MODULE,
	/// Allow ioperm/iopl access and sending USB messages to any device.
	SYS_RAWIO        = CAP_SYS_RAWIO,
	/// Allow calls to `chroot()`.
	SYS_CHROOT       = CAP_SYS_CHROOT,
	/// Allow ptrace() of any process.
	SYS_PTRACE       = CAP_SYS_PTRACE,
	/// Allow configuration of process accounting.
	SYS_PACCT        = CAP_SYS_PACCT,
	/// Allows all kinds of system administration operations.
	SYS_ADMIN        = CAP_SYS_ADMIN,
	/// Allows to call reboot().
	SYS_BOOT         = CAP_SYS_BOOT,
	/// Allows to modify scheduling priorities of own and other processes.
	SYS_NICE         = CAP_SYS_NICE,
	/// Override all kinds of resource limits.
	SYS_RESOURCE     = CAP_SYS_RESOURCE,
	/// Allow manipulation of time.
	SYS_TIME         = CAP_SYS_TIME,
	/// Allow configuration of TTY devices.
	SYS_TTY_CONFIG   = CAP_SYS_TTY_CONFIG,
	/// Allow the privileged aspects of `mknod()`.
	MKNOD            = CAP_MKNOD,
	/// Allow taking of leases on files.
	LEASE            = CAP_LEASE,
	/// Allow writing to the audit log.
	AUDIT_WRITE      = CAP_AUDIT_WRITE,
	/// Allow configuration of the audit log system.
	AUDIT_CONTROL    = CAP_AUDIT_CONTROL,
	/// Set or remove capabilities on files.
	/**
	 * This also allows to map uid=0 in a child user namespace.
	 **/
	SETFCAP          = CAP_SETFCAP,
	/// Bypass mandatory access control checks.
	/**
	 * Some MAC policies like SELinux or AppArmor may support this
	 * capability to bypass access controls.
	 **/
	MAC_OVERRIDE     = CAP_MAC_OVERRIDE,
	/// Allow configuration of mandatory access control.
	MAC_ADMIN        = CAP_MAC_ADMIN,
	/// Allow configuration of the kernel's syslog (printk) behaviour.
	SYSLOG           = CAP_SYSLOG,
	/// Allow triggering something that will wake the system.
	WAKE_ALARM       = CAP_WAKE_ALARM,
	/// Allow preventing system suspend.
	BLOCK_SUSPEND    = CAP_BLOCK_SUSPEND,
	/// Allow read access to the audit log.
	AUDIT_READ       = CAP_AUDIT_READ,
	/// Allow system performance operations and monitoring using perf_events.
	PERFMON          = CAP_PERFMON,
	/// Various Berkeley packet filter operations.
	BPF              = CAP_BPF,
	/// Checkpoint/restore related operations.
	/**
	 * - PID selection during clone3()
	 * - writing to `ns_last_pid`
	 **/
	CHECKPOINT_RESTORE = CAP_CHECKPOINT_RESTORE,
};

} // end ns
