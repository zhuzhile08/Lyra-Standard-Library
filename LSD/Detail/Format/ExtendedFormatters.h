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

#include "FormatCore.h"
#include "FormatSpecs.h"
#include "FormatContext.h"

#include "../CoreUtility.h"
#include "../../String.h"
#include "../../StringView.h"

#include <cstddef>
#include <concepts>

namespace lsd {

// container formatter
template <IteratableContainer ContainerType, class CharTy> struct Formatter<ContainerType, CharTy> {
public:
	using value_type = ContainerType;
	using char_type = CharTy;
	using back_inserter = detail::BasicFormatBackInserter<char_type>;
	using format_spec = detail::BasicFormatSpec<value_type, char_type>;
	using elem_format_spec = detail::BasicFormatSpec<typename value_type::value_type, char_type>;
	using context_type = BasicFormatContext<char_type>;

	using string_type = lsd::BasicString<char_type>;
	using view_type = lsd::BasicStringView<char_type>;

	void format(const value_type& value, context_type& context) {
		auto& inserter = context.out();

		format_spec spec(context);
		auto result = outputResult(value, inserter, spec);

		switch (spec.align) {
			case '<':
				if (inserter.done()) return;
				writeResultToOutput(result, inserter);

				for (auto count = spec.width; !inserter.done() && count > result.size(); count--) 
					inserter = spec.fillChr;

				break;

			case '>':
				for (auto count = spec.width; !inserter.done() && count > result.size(); count--) 
						inserter = spec.fillChr;

				if (!inserter.done()) writeResultToOutput(result, inserter);

				break;

			case '^':
				if (spec.width > result.size()) {
					auto fillc = spec.width - result.size();
					auto half = fillc / 2;
					
					for (auto count = fillc - half; !inserter.done() && count > 0; count--) inserter = spec.fillChr;

					if (inserter.done()) return;
					writeResultToOutput(result, inserter);

					for (; !inserter.done() && half > 0; half--) inserter = spec.fillChr;
				} else {
					if (inserter.done()) return;
					writeToOutput(value, inserter, spec);
				}

				break;
		}
	}

private:
	string_type outputResult(const value_type& value, const format_spec& spec) {
		string_type result;

		switch (spec.rangeType) {
			case detail::RangeType::altTupleFormat:
				

				break;

			case detail::RangeType::stringFormat:

				break;

			case detail::RangeType::escStringFormat:

				break;
		}
	}
	void writeElement(string_type& result, const typename value_type::value_type& elem, const elem_format_spec& spec) {

	}

	void writeResultToOutput(const string_type& result, back_inserter& inserter) {

	}
};

// special type erased argument store formatters

template <class CharTy> struct Formatter<detail::TypeErasedFormatArg<BasicFormatContext<CharTy>>, CharTy> {
	void format(
		const detail::TypeErasedFormatArg<BasicFormatContext<CharTy>>& fmtArg, 
		const BasicFormatContext<CharTy>& context) {
		fmtArg.format(context);
	}
};

} // namespace lsd
