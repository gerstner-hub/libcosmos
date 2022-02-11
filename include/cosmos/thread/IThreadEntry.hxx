#ifndef COSMOS_ITHREADENTRY_HXX
#define COSMOS_ITHREADENTRY_HXX

namespace cosmos {

class Thread;

/**
 * \brief
 *	Interface used for threads to run in
 **/
class IThreadEntry
{
public:
	//! entry function for a thread
	virtual void threadEntry(Thread &t) = 0;
};

} // end ns

#endif // inc. guard
