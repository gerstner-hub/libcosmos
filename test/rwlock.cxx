#include "cosmos/thread/RWLock.hxx"

int main()
{
	cosmos::RWLock rwl;

	rwl.readlock();
	rwl.readlock();

	rwl.unlock();
	rwl.unlock();

	rwl.writelock();
	rwl.unlock();

	return 0;
}
