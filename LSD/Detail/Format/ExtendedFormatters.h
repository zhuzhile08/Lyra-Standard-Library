/*************************
 * @file Formatters.h
 * @author Zhile Zhu (zhuzhile08@gmail.com)
 * 
 * @brief Implementation for all formatter classes
 * 
 * @date 2024-10-31
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

// Container formatter
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
				if (!writeToOutput(result, inserter)) return;

				for (auto count = spec.width; !inserter.done() && count > result.size(); count--) 
					inserter = spec.fillChr;

				break;

			case '>':
				for (auto count = spec.width; count > result.size(); count--) {
					if (inserter.done()) return;
					inserter = spec.fillChr;
				}

				writeToOutput(result, inserter);

				break;

			case '^':
				if (spec.width > result.size()) {
					auto fillc = spec.width - result.size();
					auto half = fillc / 2;
					
					for (auto count = fillc - half; count > 0; count--) {
						if (inserter.done()) return;
						inserter = spec.fillChr;
					}

					if (!writeToOutput(result, inserter)) return;

					for (; !inserter.done() && half > 0; half--) inserter = spec.fillChr;
				} else writeToOutput(value, inserter, spec);
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

			default:
				
		}

		return result;



		/*
		switch (typeOptions) {
			case '?':
				for (std::size_t i = 0; !inserter.done() && i < length; i++) {
					switch (auto c = data[i]; c) {
						case '\t':
							inserter = '\\';
							if (inserter.done()) return;
							inserter = 't';

							break;

						case '\n':
							inserter = '\\';
							if (inserter.done()) return;
							inserter = 'n';

							break;

						case '\r':
							inserter = '\\';
							if (inserter.done()) return;
							inserter = 'r';

							break;

						case '\"':
							inserter = '\\';
							if (inserter.done()) return;
							inserter = '\"';

							break;

						case '\'':
							inserter = '\\';
							if (inserter.done()) return;
							inserter = '\'';

							break;

						case '\\':
							if (++i < length) {
								if (inserter.done()) return;

								switch (c = data[i]; c) {
									case 't': inserter = '\t'; break;
									case 'n': inserter = '\n'; break;
									case 'r': inserter = '\r'; break;
									case 'b': inserter = '\b'; break;
									case 'f': inserter = '\f'; break;
									case 'a': inserter = '\a'; break;
									case 'v': inserter = '\v'; break;
									case '\\': inserter = '\\'; break;
									case '\'': inserter = '\''; break;
									case '\"': inserter = '\"'; break;
									default: inserter = 'c';
								}
							}

							break;

						default:
							inserter = c;
					}
				}

				break;

			default:
				for (std::size_t i = 0; !inserter.done() && i < length; i++) inserter = data[i];
		}
		*/
	}
	void writeElement(string_type& result, const typename value_type::value_type& elem, const elem_format_spec& spec) {

	}

	bool writeToOutput(const string_type& result, back_inserter& inserter) {
		for (auto c : result) {
			if (!inserter.done()) return false;
			inserter = c;
		}

		return true;
	}
};

// Special type erased argument store formatters

template <class CharTy> struct Formatter<detail::TypeErasedFormatArg<BasicFormatContext<CharTy>>, CharTy> {
	void format(
		const detail::TypeErasedFormatArg<BasicFormatContext<CharTy>>& fmtArg, 
		const BasicFormatContext<CharTy>& context) {
		fmtArg.format(context);
	}
};

} // namespace lsd
