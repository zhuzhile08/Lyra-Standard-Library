/************************
 * @file Iterator.hpp
 * @author Zhile Zhu (zhuzhile08@gmail.com)
 * 
 * @brief Iterator for unordered flat
 * 
 * @date 2026-07-12
 * @copyright Copyright (c) 2026
 ************************/

#pragma once

#include "Metadata.hpp"

#include <bit>
#include <iterator>

namespace lsd {

namespace unordered_flat {

template <class Ty> class Iterator {
public:
	using difference_type = std::ptrdiff_t;
	using iterator_category = std::forward_iterator_tag;

	using value_type = Ty;
	using const_value = const value_type;
	using pointer = value_type*;
	using pointer_const = const_value*;
	using reference = value_type&;

	using size_type = std::size_t;
	using uintptr_type = std::uintptr_t;

	using metadata_group = detail::MetadataGroup;
	using group_pointer = const metadata_group*;
	using const_word_pointer = metadata_group::const_word_pointer;
	using metadata_pointer = metadata_group::byte_pointer; // Won't be modified anyways

	using container = Iterator;
	using container_rvreference = Iterator&&;
	using container_reference = container&;
	using const_container_reference = const container&;

	
	constexpr Iterator() noexcept = default;
	constexpr Iterator(const_container_reference) noexcept = default;
	constexpr Iterator(container_rvreference) noexcept = default;
	constexpr Iterator(std::nullptr_t) noexcept { }

	constexpr container_reference operator=(const_container_reference) noexcept = default;
	constexpr container_reference operator=(container_rvreference) noexcept = default;

	constexpr container_reference operator++() noexcept {
		const auto index = groupIndex();
		const auto next = index + 1;
		const auto metadata = metadataArray(index);
		
		if (next < metadata_group::bucketCount - 1 && metadata_group::occupied(metadata, next)) [[likely]] {
			++m_metadata;
			++m_pointer;

			return *this;
		}

		m_pointer -= index;
		basicIncrement(metadata, metadata_group::matchOccupied(metadata) & (0b1111111111111110 << index));
	
		return *this;
	}

	constexpr container operator++(int) noexcept { 
		container tmp = *this; 
		++(*this); 
		return tmp; 
	}

	constexpr reference operator*() const {
		return *m_pointer;
	}
	constexpr pointer operator->() const noexcept {
		return m_pointer;
	}
	constexpr pointer get() const noexcept {
		return m_pointer;
	}

	constexpr operator Iterator<const_value>() const noexcept {
		return Iterator<const_value>(m_metadata, m_pointer);
	}
	constexpr explicit operator pointer() noexcept {
		return m_pointer;
	}
	constexpr explicit operator pointer_const() const noexcept {
		return m_pointer;
	}

	friend constexpr bool operator==(const_container_reference first, const_container_reference second) noexcept {
		return first.m_pointer == second.m_pointer;
	}

private:
	metadata_pointer m_metadata = nullptr;
	pointer m_pointer = nullptr;


	constexpr Iterator(metadata_pointer metadata, pointer pointer) noexcept :
		m_metadata(metadata), m_pointer(pointer) { }
	constexpr Iterator(group_pointer metadata, pointer pointer) noexcept :
		m_metadata(reinterpret_cast<metadata_pointer>(metadata)), m_pointer(pointer)
	{
		if (!m_pointer)
			return;

		// This function is only called during the creation of begin iterators, therefore if a the first element is set, we can just continue
		if (metadata->occupied(0))
			return;

		basicIncrement(metadataArray(groupIndex()), metadata->matchOccupied());
	}

	constexpr void basicIncrement(const_word_pointer metadata, size_type occupied) noexcept {
		if (occupied == 0) {
			do {
				metadata += metadata_group::groupWordSize;
				m_pointer += metadata_group::bucketCount;
			} while (metadata_group::empty(metadata));

			occupied = metadata_group::matchOccupied(metadata);
		}

		const auto index = std::countr_zero(occupied);
		if (metadata_group::isSentinel(metadata, index)) [[unlikely]]
			m_pointer = nullptr;
		else [[likely]] {
			m_metadata = reinterpret_cast<metadata_pointer>(metadata) + index;
			m_pointer += index;
		}
	}

	constexpr size_type groupIndex() const noexcept {
		return reinterpret_cast<uintptr_type>(m_metadata) & (metadata_group::groupSize - 1);
	}
	constexpr metadata_group::word_pointer metadataArray(size_type index) noexcept {
		return const_cast<metadata_group::word_pointer>(reinterpret_cast<const_word_pointer>(m_metadata - index));
	}
	constexpr const_word_pointer metadataArray(size_type index) const noexcept {
		return reinterpret_cast<const_word_pointer>(m_metadata - index);
	}
	
	template <class, class, class, class, class> friend class UnorderedFlatCore;
	template <class, class, class, class, class> friend class BasicUnorderedFlat;
	template <class> friend class Iterator;
};

} // namespace unordered_flat

} // namespace lsd
