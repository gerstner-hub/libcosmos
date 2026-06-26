#pragma once

// Linux
#include <sys/auxv.h>

// C++
#include <cstddef>
#include <map>
#include <optional>
#include <span>
#include <string_view>
#include <variant>
#include <vector>

// cosmos
#include <cosmos/creds.hxx>
#include <cosmos/dso_export.h>
#include <cosmos/fs/types.hxx>

namespace cosmos {

/// Parser for Linux auxiliary vector data.
/**
 * The auxiliary vector is data associated with a Linux process containing a
 * range of key/value pairs describing process properties. The raw data can be
 * obtained via prctl::get_aux_vector() or by reading `/proc/<pid>/auxv`. The
 * vector consists of binary data, which can be parsed and accessed by this
 * class.
 *
 * Each entry in the vector can be of one of a number of types, resulting in
 * string data, a code pointer, an integer or other data format. A list of
 * well-known types is available via the AuxVector::Type enum. When accessing
 * a type a std::variant called AuxVector::Data is returned containing the
 * value as a strong type.
 *
 * This class can either operate on a view of external data, in which case the
 * caller must ensure that the external data remains valid as long as
 * necessary, or the auxiliary data can be copied into the class instance,
 * which is more secure but less efficient. Further a move constructor exists
 * to transfer ownership of external data into this class.
 **/
class COSMOS_API AuxVector {
public: // types

	/// Available type entries for the auxiliary vector.
	/**
	 * Not every Type is necessarily present in the AuxVector, which is
	 * why a std::nullopt can be returned when trying to access it.
	 **/
	enum class Type : unsigned long {
		/// Base address of program interpreter (dynamic linker).
		BASE = AT_BASE,
		/// String identifying the real platform (PowerPC) or ISA level (MIPS)
		BASE_PLATFORM = AT_BASE_PLATFORM,
		/// Frequency with which times(2) counts.
		CLKTCK = AT_CLKTCK,
		/// Data cache block size
		DCACHEBSIZE = AT_DCACHEBSIZE,
		/// The effective group ID of the (main) thread.
		EGID = AT_EGID,
		/// Entry point address of the executable.
		ENTRY = AT_ENTRY,
		/// The effective user ID of the (main) thread.
		EUID = AT_EUID,
		/// File descriptor of the program.
		EXECFD = AT_EXECFD,
		/// String containing the pathname used to execute the program.
		EXECFN = AT_EXECFN,
		/// The real group ID of the (main) thread.
		GID = AT_GID,
		/// Instruction cache block size.
		ICACHEBSIZE = AT_ICACHEBSIZE,
		/// Geometry of the L1 data cache.
		/**
		 * All cache geometries are encoded as follows:
		 *
		 * - the cache line size is found in the bottom 16 bits and
		 *   cache associativity in the next 16 bits. If N if in the
		 *   second 16 bits then the cache is N-way set associative.
		 **/
		L1D_CACHEGEOMETRY = AT_L1D_CACHEGEOMETRY,
		/// The L1 data cache size.
		L1D_CACHESIZE = AT_L1D_CACHESIZE,
		/// Geometry of the L1 instruction cache.
		L1I_CACHEGEOMETRY = AT_L1I_CACHEGEOMETRY,
		/// The L1 instruction cache size.
		L1I_CACHESIZE = AT_L1I_CACHESIZE,
		/// Geometry of the L2 cache.
		L2_CACHEGEOMETRY = AT_L2_CACHEGEOMETRY,
		/// The L2 cache size.
		L2_CACHESIZE = AT_L2_CACHESIZE,
		/// The L3 cache geometry.
		L3_CACHEGEOMETRY = AT_L3_CACHEGEOMETRY,
		/// The system page size.
		PAGESZ = AT_PAGESZ,
		/// Address of the program headers of the executable.
		PHDR = AT_PHDR,
		/// The size of the program header entry at PHDR.
		PHENT = AT_PHENT,
		/// String that identifies the hardware platform that the program is running on.
		PLATFORM = AT_PLATFORM,
		/// Pointer to 16 bytes containing a random value.
		RANDOM = AT_RANDOM,
		/// Non-zero if the executable should be treated securely.
		SECURE = AT_SECURE,
		/// Entry-point to the system call function in the vDSO if present on the architecture.
		SYSINFO = AT_SYSINFO,
		/// The address of a page containing the vDSO setup by the kernel.
		SYSINFO_EHDR = AT_SYSINFO_EHDR,
		/// Unified cache block size.
		UCACHEBSIZE = AT_UCACHEBSIZE,
		/// The real user ID of the (main) thread.
		UID = AT_UID,
	};

	using enum Type;

	using Data = std::variant<
		std::string_view, /* any string pointers */
		unsigned long, /* any numbers */
		void*, /* any other code / data pointers */
		cosmos::FileNum, /* for EXECFD */
		std::span<const std::byte>, /* for RANDOM */
		cosmos::UserID, /* UID, EUID */
		cosmos::GroupID, /* GID, EGID */
		bool /* for SECURE */
	>;

public: // functions

	/// Operate on the given span without copying.
	/**
	 * Ownership of the data is not transferred, the caller is responsible
	 * to ensure that `data` stays valid for the lifetime of the
	 * AuxVector.
	 **/
	explicit AuxVector(std::span<const std::byte> data) :
		m_view{data} {
	}

	/// Move constructor to transfer ownership into AuxVector.
	/**
	 * This takes ownership of `buffer` for the purposes of processing the
	 * auxiliary vector data.
	 **/
	explicit AuxVector(std::vector<std::byte> &&buffer) :
		m_buffer{std::move(buffer)},
		m_view{m_buffer.data(), m_buffer.size()} {
	}

	/// Copy the given `buffer` to operate on.
	/**
	 * This will perform a deep copy of the given `buffer` to give the
	 * AuxVector ownership of a dedicated instance of the data.
	 **/
	explicit AuxVector(const std::vector<std::byte> &buffer) :
		m_buffer{buffer},
		m_view{buffer.data(), buffer.size()} {
	}

	/// Copy the given plain buffer into AuxVector.
	/**
	 * To support operating on arbitrary data sources (not available as
	 * std::vector) this constructor supports copying of arbitrary source
	 * data into the object.
	 **/
	explicit AuxVector(const std::byte *bytes, const size_t size) {
		for (size_t i = 0; i < size; i++) {
			m_buffer.push_back(bytes[i]);
		}

		m_view = std::span<const std::byte>{m_buffer.data(), m_buffer.size()};
	}

	/// Obtain the auxiliary vector data as a std::map.
	/**
	 * The returned map can be used both for iterating over all existing
	 * data and for efficiently looking up specific data items.
	 **/
	std::map<Type, Data> asMap() const;

	/// Lookup the given `type` in the vector and return its data.
	/**
	 * This can only return Data when `type` actually exists in the
	 * vector. If it doesn't exist then std::nullopt is returned here.
	 **/
	std::optional<Data> lookupValue(const Type type) const;

protected: // data

	/// internal auxv storage in case we own the data.
	std::vector<std::byte> m_buffer;
	/// view onto external or internal auxv buffer.
	std::span<const std::byte> m_view;
};

} // end ns
