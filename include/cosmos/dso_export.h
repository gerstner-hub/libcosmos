#ifndef COSMOS_EXPORT_HXX
#define COSMOS_EXPORT_HXX

/*
 * Helper macro to control visibility of public APIs of the cosmos shared
 * library.
 * The object files are compiled with -fvisibility=hidden, thus each public
 * interface needs to be explicitly made visible again.
 */
#if defined(COSMOS_EXPORT)
#	define COSMOS_API __attribute__ ((visibility ("default")))
#else
#	define COSMOS_API
#endif

#endif // inc. guard
