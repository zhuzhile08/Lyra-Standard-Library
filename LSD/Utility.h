/*************************
 * @file Utility.h
 * @author Zhile Zhu (zhuzhile08@gmail.com)
 * 
 * @brief Miscelaneous utility functions
 * 
 * @date 2022-02-03
 * 
 * @copyright Copyright (c) 2022
 *************************/

#pragma once

#include "Iterators.h"
#include "Detail/CoreUtility.h"

#include <limits>
#include <functional>

#include <cstring>

#include <system_error>

namespace lsd {

// Address conversion
template <class Ty> [[nodiscard]] constexpr inline const void* getAddress(const Ty& type) noexcept {
	return static_cast<const void*>(type);
}


// Compile time type id generator

using type_id = const void*;

template <class> struct TypeId {
private:
	constexpr static char m_id { };

	template <class> friend constexpr type_id typeId() noexcept;
};

template <class Ty> constexpr type_id typeId() noexcept {
	return &TypeId<Ty>::m_id;
}

} // namespace lsd


namespace std {

// Standard library function overloads for custom containers

template <lsd::IteratableContainer Ty> void swap(Ty& a, Ty& b) {
	a.swap(b);
}

template <lsd::IteratableContainer Ty, class Value = typename Ty::value_type> Ty::size_type erase(Ty& container, const Value& value) {
	auto it = std::remove(container.begin(), container.end(), value);
	auto r = container.end() - it;
	container.erase(it, container.end());
	return r;
}
template <lsd::IteratableContainer Ty, class Pred> Ty::size_type erase(Ty& container, Pred pred) {
	auto it = std::remove(container.begin(), container.end(), pred);
	auto r = container.end() - it;
	container.erase(it, container.end());
	return r;
}

} // namespace std
