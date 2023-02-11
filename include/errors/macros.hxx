#ifndef COSMOS_ERRORS_MACROS_HXX
#define COSMOS_ERRORS_MACROS_HXX

// libcosmos
#include "cosmos/compiler.hxx"

/**
 * @file
 * This header contains exception handling related macros specific to
 * libcosmos's exception model.
 **/

// NOTE: once we have C++20 we can use std::source_location for this
// PRETTY_FUNCTION includes C++ context like class and signature, is available
// in gcc (maybe also clang?) but it is *not* a preprocessor define so we need
// to check for the compiler here to detect support for it.

#ifdef COSMOS_GCC
/// Throws the given Exception type after information from the calling context has been added
#	define cosmos_throw(e) (e.setInfo(__FILE__, __LINE__, __PRETTY_FUNCTION__).raise())
#else
#	define cosmos_throw(e) (e.setInfo(__FILE__, __LINE__, __FUNCTION__).raise())
#endif
/// Use this in each type derived from CosmosError to apply mandatory overrides
/**
 * since this is a virtual function, the `noreturn` attribute is not enough to
 * silence "no return in function returning non-void" functions. I didn't find
 * a way around that yet, meaning that somethings unnecessary returns have to
 * be added to silence these warnings.
 **/
#define COSMOS_ERROR_IMPL [[ noreturn ]] void raise() override { throw *this; }

#endif // inc. guard
