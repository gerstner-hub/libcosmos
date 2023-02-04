#ifndef COSMOS_INIT_HXX
#define COSMOS_INIT_HXX

namespace cosmos {

/**
 * @file
 *
 * This header contains the API for global library initialization.
 **/

/// Initializes the cosmos library before first use
/**
 * The initialization of the library is required before any other
 * functionality of libcosmos is accessed. This initialization should occur
 * after the main() function has been entered and not from within static
 * initializers to avoid issues with static initialization order in
 * executables / libraries.
 *
 * Multiple initializations can be performed but finish() needs to be
 * called the same number of times for cleanup to occur.
 **/
void COSMOS_API init();

/// Undo initialization of init()
void COSMOS_API finish();

/// Convenience initialization object
/**
 * During the lifetime of this object the cosmos library remains initialized.
 **/
struct Init {
	Init() { init(); }

	~Init() { finish(); }
};

} // end ns

#endif // inc. guard
