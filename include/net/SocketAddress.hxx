#pragma once

// C++
#include <cstring>

// cosmos
#include "cosmos/dso_export.h"
#include "cosmos/net/types.hxx"

namespace cosmos {

/// Base class for all types of socket addresses.
/**
 * This is the equivalent of the raw `sockaddr*` type. It needs to be derived
 * from to form a concrete address type. This type is used for passing to
 * generic functions like Socket::bind and Socket::connect.
 *
 * A couple of pure virtual functions need to be implemented by concrete
 * address types. This is necessary to deal with system calls in a generic
 * way.
 *
 * When passed as an input parameter then size() determines the actual
 * number of bytes used for the address. Some address types always have the
 * same fixed size while some like UnixAddress can use dynamic sizes.
 *
 * When passed as an output parameter then maxSize() determines the maximum
 * number of bytes the kernel can use for writing address information to.
 * After a system call has filled in the address structure update() will be
 * called to allow the implementation to inspect the new data an possibly
 * adjust further object state.
 **/
class COSMOS_API SocketAddress {
	friend class Socket;
	friend class SendMessageHeader;
	friend class ReceiveMessageHeader;
public: // functions

	virtual ~SocketAddress() {}

	//// Returns the concrete SocketFamily for the implementation address type.
	virtual SocketFamily family() const = 0;

	/// Returns the size of the socket address in bytes found at basePtr().
	/**
	 * This returns the number of bytes currently used in the socket
	 * address. Some implementations may have dynamic sizes in which case
	 * this can differ from maxSize().
	 **/
	virtual size_t size() const = 0;

	/// Returns the maximum number of bytes the socket address can hold.
	/**
	 * If the concrete socket address type can have dynamic size then this
	 * returns the maximum number of bytes the structure found at
	 * basePtr() can hold.
	 **/
	virtual size_t maxSize() const {
		return size();
	}

protected: // functions

	/// Returns a mutable pointer to the `sockaddr*` base structure.
	virtual sockaddr* basePtr() = 0;
	/// Returns a const pointer to the `sockaddr*` base structure.
	virtual const sockaddr* basePtr() const = 0;

	/// Clears the complete address structure.
	/**
	 * After this call the complete address structure found at basePtr()
	 * will overwritten by zeroes, and the family field will be
	 * initialized with the value returned from family().
	 *
	 * This uses the length information returned by maxSize() for zeroing
	 * out the structure..
	 **/
	void clear();

	/// Update the address structure after it has been filled in by the kernel.
	/**
	 * Some system calls update the address structure. The obtained number
	 * of bytes is found in `new_length`. Some address structures have
	 * dynamic sizes in which case the new length has to be accomodated
	 * for by overriding this function call.
	 *
	 * The default implementation only performs a sanity check that the
	 * new length matches the old length (considering statically sized
	 * addresses).
	 **/
	virtual void update(size_t new_length);
};

}; // end ns
