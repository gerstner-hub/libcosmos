#ifndef COSMOS_COMPILER_HXX
#define COSMOS_COMPILER_HXX

#include <iosfwd>

#if defined(__GLIBCXX__)
#       define COSMOS_GNU_CXXLIB
#elif defined(_LIBCPP_VERSION)
#       define COSMOS_LLVM_CXXLIB
#else
#       error "Couldn't determine the kind of C++ standard library"
#endif

#endif // inc. guard
