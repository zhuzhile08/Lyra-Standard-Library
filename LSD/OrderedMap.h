/*************************
 * @file OrderedMap.h
 * @author Zhile Zhu (zhuzhile08@gmail.com)
 * 
 * @brief Ordered map implemented using a red-black tree
 * 
 * @date 2024-05-14
 * @copyright Copyright (c) 2024
 *************************/

#pragma once

#include "Utility.h"
#include "Iterators.h"
#include "Iterators.h"

#include <initializer_list>
#include <functional>
#include <utility>

namespace lsd {

template <class Key, class Ty, class Compare, class Allocator> class OrderedMap {
public:
	using mapped_type = Ty;
	using key_type = Key;
	using allocator_type = Alloc;
	template <class F, class S> using pair_type = std::pair<F, S>;

	using value_type = pair_type<key_type, mapped_type>;
	using reference = value_type&;
	using const_value = const value_type;
	using const_reference = const_value&;
	using rvreference = value_type&&;
	using pointer = value_type*;
	using const_pointer = const pointer;

	using iterator = typename OrderedMapIterator<value_type>;
	using const_iterator = typename OrderedMapIterator<const_value>;

	using hasher = Hash;
	using key_equal = Equal;

	using wrapper = OrderedMap;
	using const_wrapper_reference = const wrapper&;
	using wrapper_rvreference = wrapper&&;

private:

};

} // namespace lsd
