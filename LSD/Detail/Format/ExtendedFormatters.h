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

namespace detail {

enum class RangeType {
	altTupleFormat,
	stringFormat,
	escStringFormat
};

template <class CharTy> struct ContainerFormatSpec {
public:
	using char_type = CharTy;

	constexpr ContainerFormatSpec(const detail::BasicFieldOptions<CharTy>& options) {
		auto it = options.formatSpec.begin();
		auto end = options.formatSpec.end();

		switch (*it) { // parse early exit end alignment spec
			case '}':
				return;

				break;

			default: {
				auto alignFirst = it;

				switch (*++it) {
					case '<':
					case '>':
					case '^':
						align = *it;
						fillChr = *alignFirst;
						++it;

						break;

					default:
						switch (*alignFirst) {
							case '<':
							case '>':
							case '^':
								align = *alignFirst;
								++it;

								break;
							
							default:
								it = alignFirst;
								
								break;
						}
				}

				break;
			}
		}

		// closing brackets options
		if (*it == 'n') {
			closingBrackets = false;
			++it;
		}

		switch (*it) { // parse range type and early exit
			case '}':
				return;

			default: {
				switch (*it) {
					case 'm':
						rangeType = RangeType::altTupleFormat;

						break;

					case 's':
						rangeType = RangeType::stringFormat;

						break;

					case '?':
						rangeType = RangeType::escStringFormat;
						++it;

						break;
				}

				break;
			}
		}

		if (*++it == ':') underlyingSpec = BasicStringView<CharTy>(++it, end);
	}

	char_type fillChr = ' ';
	char_type align = '<';

	bool closingBrackets = true;
	RangeType rangeType;

	BasicStringView<char_type> underlyingSpec;
};

} // namespace detail


// container formatter
template <IteratableContainer ContainerType, class CharTy> struct Formatter<ContainerType, CharTy> {
public:
	using value_type = ContainerType;
	using char_type = CharTy;
	using back_inserter = detail::BasicFormatBackInserter<char_type>;
	using format_spec = detail::BasicFieldOptions<char_type>;

	using string_type = lsd::BasicString<char_type>;
	using view_type = lsd::BasicStringView<char_type>;

	void format(const value_type& value, back_inserter& inserter, const format_spec& options) {
		format_spec spec(options);
		auto length = outputLength(value, options);

		switch (spec.align) {
			case '<':
				if (inserter.done()) return;
				writeToOutput(value, inserter, spec);

				for (auto count = spec.width; !inserter.done() && count > length; count--) 
					inserter = spec.fillChr;

				break;

			case '>':
				for (auto count = spec.width; !inserter.done() && count > length; count--) 
						inserter = spec.fillChr;

				if (!inserter.done()) writeToOutput(value, inserter, spec);

				break;

			case '^':
				if (spec.width > length) {
					auto fillc = spec.width - length;
					auto half = fillc / 2;
					
					for (auto count = fillc - half; !inserter.done() && count > 0; count--) inserter = spec.fillChr;

					if (inserter.done()) return;
					writeToOutput(value, inserter, spec);

					for (; !inserter.done() && half > 0; half--) inserter = spec.fillChr;
				} else {
					if (inserter.done()) return;
					writeToOutput(value, inserter, spec);
				}

				break;
		}
	}

private:
	void writeToOutput(const value_type& value, back_inserter& inserter, const format_spec& options) {

	}

	void writeElement(const value_type::value_type& value, back_inserter& inserter, const view_type& rawSpec) {

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
