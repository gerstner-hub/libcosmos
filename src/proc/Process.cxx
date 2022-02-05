// Cosmos
#include "cosmos/proc/Process.hxx"

// Linux
#include <sys/types.h>
#include <unistd.h>

namespace cosmos
{

ProcessID Process::cachePid() const
{
	m_own_pid = ::getpid();
	return m_own_pid;
}

ProcessID Process::cachePPid() const
{
	m_parent_pid = ::getppid();
	return m_parent_pid;
}

UserID Process::getRealUserID() const
{
	return ::getuid();
}

UserID Process::getEffectiveUserID() const
{
	return ::geteuid();
}

Process g_process;

} // end ns
