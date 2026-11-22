/************************
 * @file Metadata.hpp
 * @author Zhile Zhu (zhuzhile08@gmail.com)
 * 
 * @brief Metadata group for unordered flat
 * 
 * @date 2026-07-12
 * @copyright Copyright (c) 2026
 ************************/

#pragma once

#include "../../Array.h"

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <functional>


#define LSD_UNORDERED_FLAT_TO_BYTE(metadata) reinterpret_cast<metadata_group::byte_pointer>(metadata)

namespace lsd {

namespace unordered_flat {

namespace detail {

/**
 * Huuuge thank you to https://github.com/joaquintides for explaining the super awesome
 * algorithm for the hash matching. I could NOT figure this out until you explained the
 * algorithm behind this step by step, really appreciate it a lot!
 *
 * If Mr. Muñoz happens to stumble across this, again, thank you very much, you're an
 * awesome guy, and please keep up your contributions for the open source community!
 *
 * The metadata of the unordered flat map uses 16 bytes to store information for 15
 * elements with one control byte for hash map operations. The metadata stores the lower
 * 7 bits of the hash of the object. Instead of laying out the hash linearly, their bits
 * are interleaved - the metadata bits of the n-th element will be contained at every
 * n-th bit of every 16-bit group. This is done to optimize for non-simd matching
 *
 * The topmost bit of each bit-pattern-segment is used to mark the overflow status of
 * the element with a hash, whose mod 8 is the index of the segment, as the concrete
 * location of the overflow bit does not have to coincide with the actual pattern, since
 * it is not used in lookup
 *
 * The metadata values 0x00 and 0x01 are reserved, as one represents an empty bucket
 * available for reinsertion, and the other represents a sentinel to end iteration.
 * Therefore, the hash may not be directly placed into the metadata array. As the 8th
 * modulo of the hash is needed for fast overflow status lookup, a lookup array exists
 * to map the hash to an equivalent number, whose 8th mod produces the same result
 */

class MetadataGroup {
public:
	using size_type = std::size_t;

	using word_type = std::uint64_t;
	using word_pointer = word_type*;
	using const_word_pointer = const word_type*;

	using byte_type = std::uint8_t;
	using byte_pointer = const byte_type*;

	using container = MetadataGroup;


	static constexpr size_type available = 0x00;

	static constexpr size_type groupWordSize = 2;

	static constexpr size_type bucketCount = 15; // Bucket size refers to the actual elements stored
	static constexpr size_type groupSize = 16; // Group size refers to the total byte count of the group

	static constexpr size_type full = 0x7FFF;

private:
	static constexpr size_type sentinel = 0x01;
	static constexpr size_type sentinelIndex = 14;

	static constexpr auto occupiedBitPattern = UINT64_C(0x0001000100010001);

public:

	// Metadata functions

	constexpr void insert(size_type index, size_type shortHash) noexcept {
		basicInsert(m_metadata.data(), index, shortHash);
	}
	constexpr void erase(size_type index) noexcept {
		container::erase(m_metadata.data(), index);
	}
	constexpr bool occupied(size_type index) const noexcept {
		return container::occupied(m_metadata.data(), index);
	}


	// Sentinel functions

	constexpr void insertSentinel() noexcept {
		basicInsert(m_metadata.data(), sentinelIndex, sentinel);
	}
	constexpr bool isSentinel(size_type index) const noexcept {
		return container::isSentinel(m_metadata.data(), index);
	}

	constexpr void clearAndSetSentinel() noexcept {
		m_metadata[0] = 1 << sentinelIndex;
		m_metadata[1] = 0;
	}


	// Overflow functions

	constexpr void markOverflow(size_type hash) noexcept {
		reinterpret_cast<uint16_t*>(m_metadata.data())[hash & 7] |= 0x8000;
	}
	constexpr void clearOverflow() noexcept {
		static constexpr word_type clearOverflowPattern = UINT64_C(0x7FFF7FFF7FFF7FFF);
		
		m_metadata[0] &= clearOverflowPattern;
		m_metadata[1] &= clearOverflowPattern;
	}
	constexpr bool overflowed(size_type hash) const noexcept {
		return reinterpret_cast<const uint16_t*>(m_metadata.data())[hash & 7] & 0x8000;
	}


	// Lookup functions

	constexpr size_type match(size_type shortHash) const noexcept {
		static constexpr lsd::Array<word_type, 16> multiplexBitPattern = { // Short hash to bit pattern, covering every bit of a interleaved slot
			UINT64_C(0x0000000000000000), UINT64_C(0x000000000000FFFF), UINT64_C(0x00000000FFFF0000), UINT64_C(0x00000000FFFFFFFF),
			UINT64_C(0x0000FFFF00000000), UINT64_C(0x0000FFFF0000FFFF), UINT64_C(0x0000FFFFFFFF0000), UINT64_C(0x0000FFFFFFFFFFFF),
			UINT64_C(0xFFFF000000000000), UINT64_C(0xFFFF00000000FFFF), UINT64_C(0xFFFF0000FFFF0000), UINT64_C(0xFFFF0000FFFFFFFF),
			UINT64_C(0xFFFFFFFF00000000), UINT64_C(0xFFFFFFFF0000FFFF), UINT64_C(0xFFFFFFFFFFFF0000), UINT64_C(0xFFFFFFFFFFFFFFFF)
		};

		// The only bits set will be the ones which perfectly map to the short hash pattern
		return foldWord<true>(~(m_metadata[0] ^ multiplexBitPattern[shortHash & 0xF]) & 
							  ~(m_metadata[1] ^ multiplexBitPattern[shortHash >> 4]));
	}

	constexpr std::uint32_t matchOccupied() const noexcept {
		return container::matchOccupied(m_metadata.data());
	}


	// Utility functions

	static constexpr size_type hashToMetadata(size_type hash) noexcept {
		static constexpr lsd::Array<byte_type, 256> validReducedHashes {
			0x08, 0x09, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
			0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F,
			0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F,
			0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F,
			0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F,
			0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5A, 0x5B, 0x5C, 0x5D, 0x5E, 0x5F,
			0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6A, 0x6B, 0x6C, 0x6D, 0x6E, 0x6F,
			0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7A, 0x7B, 0x7C, 0x7D, 0x7E, 0x7F,
			0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8A, 0x8B, 0x8C, 0x8D, 0x8E, 0x8F,
			0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9A, 0x9B, 0x9C, 0x9D, 0x9E, 0x9F,
			0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7, 0xA8, 0xA9, 0xAA, 0xAB, 0xAC, 0xAD, 0xAE, 0xAF,
			0xB0, 0xB1, 0xB2, 0xB3, 0xB4, 0xB5, 0xB6, 0xB7, 0xB8, 0xB9, 0xBA, 0xBB, 0xBC, 0xBD, 0xBE, 0xBF,
			0xC0, 0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8, 0xC9, 0xCA, 0xCB, 0xCC, 0xCD, 0xCE, 0xCF,
			0xD0, 0xD1, 0xD2, 0xD3, 0xD4, 0xD5, 0xD6, 0xD7, 0xD8, 0xD9, 0xDA, 0xDB, 0xDC, 0xDD, 0xDE, 0xDF,
			0xE0, 0xE1, 0xE2, 0xE3, 0xE4, 0xE5, 0xE6, 0xE7, 0xE8, 0xE9, 0xEA, 0xEB, 0xEC, 0xED, 0xEE, 0xEF,
			0xF0, 0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8, 0xF9, 0xFA, 0xFB, 0xFC, 0xFD, 0xFE, 0xFF
		};

		return validReducedHashes[hash & 0xFF];
	}


	// Getters
	
	constexpr const_word_pointer data() const noexcept {
		return m_metadata.data();
	}


	// Static versions of some functions
	
	static constexpr void erase(word_pointer metadata, size_type index) noexcept {
		const auto erasePattern = ~(occupiedBitPattern << index);
	
		metadata[0] = (metadata[0] & erasePattern);
		metadata[1] = (metadata[1] & erasePattern);
	}

	static constexpr bool occupied(const_word_pointer metadata, size_type index) noexcept {
		return ((metadata[0] | metadata[1]) & occupiedBitPattern << index) != 0; // Any occupied slot will have one of its bytes set to a non-zero value
	}

	static constexpr size_type matchOccupied(const_word_pointer metadata) noexcept {
		return foldWord<false>(metadata[0] | metadata[1]);
	}


	static constexpr size_type empty(const_word_pointer metadata) noexcept {
		return ((metadata[0] | metadata[1]) & UINT64_C(0x7FFF7FFF7FFF7FFF)) == 0;
	}


	static constexpr bool isSentinel(const_word_pointer metadata, size_type index) noexcept {
		static constexpr auto pattern = occupiedBitPattern << sentinelIndex;

		if (index != sentinelIndex)
			return false;

		const auto bottomSet = (metadata[0] & pattern) == 0x4000;
		const auto topEmpty = (metadata[1] & pattern) == 0;

		return topEmpty && bottomSet;
	}

	static constexpr size_type maybeCausedOverflow(const_word_pointer metadata, size_type index) noexcept {
		const auto rawHash = (*metadata >> index) & UINT64_C(0x0000000100010001);
		const auto hash = rawHash | (rawHash >> 15) | (rawHash >> 30);

		return metadata[hash & 7] >> 15;
	}

private:
	alignas(16) lsd::Array<word_type, groupWordSize> m_metadata { 0, 0 }; // Enforce 16 byte alignment even on 32 bit systems


	// Directly insert a single, already processed metadata element into the metadata block
	static constexpr void basicInsert(word_pointer metadata, size_type index, size_type value) noexcept {
		assert((index <= sentinelIndex) && "lsd::unordered_flat::MetadataGroup::basicInsert(): Index exceeded legal maximum group index!");

		// These patterns represent the interleaved bits of the metadata byte with the value of their index
		static constexpr lsd::Array<word_type, 16> interleavedBitPattern = {
			UINT64_C(0x0000000000000000), UINT64_C(0x0000000000000001), UINT64_C(0x0000000000010000), UINT64_C(0x0000000000010001),
			UINT64_C(0x0000000100000000), UINT64_C(0x0000000100000001), UINT64_C(0x0000000100010000), UINT64_C(0x0000000100010001),
			UINT64_C(0x0001000000000000), UINT64_C(0x0001000000000001), UINT64_C(0x0001000000010000), UINT64_C(0x0001000000010001),
			UINT64_C(0x0001000100000000), UINT64_C(0x0001000100000001), UINT64_C(0x0001000100010000), UINT64_C(0x0001000100010001)
		};

		const auto clear = ~(occupiedBitPattern << index);

		const auto lowBits = interleavedBitPattern[value & 0xF] << index;
		const auto highBits = interleavedBitPattern[value >> 4] << index;

		// First, the the bits are direcly or-ed into the metadata, but it might overlap with old data
		// This is resolved by clearing the result with the clearing pattern | bit pattern

		metadata[0] = metadata[0] & clear | lowBits;
		metadata[1] = metadata[1] & clear | highBits;
	}

	template <bool Strict> static constexpr size_type foldWord(word_type word) noexcept {
		if constexpr (Strict) {
			word &= word >> 32;
			return static_cast<size_type>(word & (word >> 16)) & full;
		} else {
			word |= word >> 32;
			return static_cast<size_type>(word | (word >> 16)) & full;
		}
	}
};

} // namespace detail

} // namespace unordered_flat

} // namespace lsd
