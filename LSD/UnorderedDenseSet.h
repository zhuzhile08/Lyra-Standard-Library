/*************************
 * @file UnorderedDenseSet.hpp
 * @author Zhile Zhu (zhuzhile08@gmail.com)
 * 
 * @brief Unordered dense set
 * 
 * @date 2024-02-24
 * @copyright Copyright (c) 2024
 *************************/

#pragma once

#include "Detail/HashTables/BasicUnorderedDense.hpp"
#include "Hash.h"

#include <memory>

namespace lsd {

template <
	class Key,
	class Hash = Hash<Key>,
	class Equal = EqualTo<Key>,
	class Alloc = std::allocator<Key>
> using UnorderedDenseSet = unordered_dense::BasicUnorderedDense<Key, void, Hash, Equal, Alloc>;

} // namespace lsd
