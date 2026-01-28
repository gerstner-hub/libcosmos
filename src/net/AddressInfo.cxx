// cosmos
#include <cosmos/net/AddressInfo.hxx>
#include <cosmos/net/AddressHints.hxx>

namespace cosmos {

// this is only a thin C++ wrapper around struct addrinfo and needs to stay
// size compatible to allow casting between the two for iterating over C style
// lists returned from getaddrinfo().
static_assert(sizeof(AddressHints) == sizeof(addrinfo));

} // end ns
