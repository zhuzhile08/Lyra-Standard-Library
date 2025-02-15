/*************************
 * @file OrderedMapNode.h
 * @author Zhile Zhu (zhuzhile08@gmail.com)
 * 
 * @brief Node of ordered map/red-black tree implementation
 * 
 * @date 2024-05-14
 * @copyright Copyright (c) 2024
 *************************/

#pragma once

namespace lsd {

namespace detail {

template <class Ty, class Alloc> class OrderedTreeNode {
	enum class Color : char {
		red,
		black
	};

	using value_type = Ty;
	using pointer = value_type*;

	constexpr OrderedTreeNode() requires std::is_default_constructible_v<value_type> = default;
	template <class... Args> constexpr OrderedTreeNode(pointer parent, Args&&... args) noexcept : parent(parent), value(std::forward<Args>(args)...) { }

	constexpr void rotateLeft() {
		
	}
	constexpr void rotateRight() {
		
	}

	value_type value;

	pointer parent { };
	pointer left { };
	pointer right { };
};

} // namespace detail

} // namespace lsd
