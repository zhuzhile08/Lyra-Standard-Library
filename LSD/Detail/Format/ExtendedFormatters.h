/*************************
 * @file Formatters.h
 * @author Zhile Zhu (zhuzhile08@gmail.com)
 * 
 * @brief Implementation for all formatter classes
 * 
 * @date 2024-10-31
 * 
 * @copyright Copyright (c) 2024
 *************************/

#pragma once

#include "../CoreUtility.h"
#include "../../String.h"
#include "../../StringView.h"

#include "FormatCore.h"

#include <cstddef>
#include <concepts>

namespace lsd {

// container formatter
template <IteratableContainer ContainerType, class CharTy> struct Formatter<ContainerType, CharTy> {
	using value_type = ContainerType;
	using char_type = CharTy;
	using back_inserter = detail::BasicFormatBackInserter<char_type>;
	using format_spec = detail::BasicFieldOptions<char_type>;

	using string_type = lsd::BasicString<char_type>;
	using view_type = lsd::BasicStringView<char_type>;

	//void format(const )
};

// special type erased argument store formatters

template <class CharTy> struct Formatter<detail::TypeErasedFormatArg<CharTy, detail::BasicFormatBackInserter<CharTy>>, CharTy> {
	void format(
		const detail::TypeErasedFormatArg<CharTy, detail::FormatBackInserter>& fmtArg, 
		detail::BasicFormatBackInserter<CharTy>& inserter, 
		const detail::BasicFieldOptions<CharTy>& spec) {
		fmtArg.format(inserter, spec);
	}
};

} // namespace lsd
