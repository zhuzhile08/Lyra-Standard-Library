/************************
 * @file FormatSpecs.h
 * @author Zhile Zhu (zhuzhile08@gmail.com)
 * 
 * @brief Parsers for the different types of format specializations
 * 
 * @date 2024-11-08
 * @copyright Copyright (c) 2024
 ************************/

#pragma once

#include "FormatCore.h"
#include "FormatContext.h"

#include "../../StringView.h"

namespace lsd {

namespace detail {

// basic format spec

template <class CharTy> struct BasicGeneralFormatSpec {
public:
	using char_type = CharTy;
	using context_type = BasicFormatContext<char_type>;

	constexpr BasicGeneralFormatSpec(const context_type& context) {
		auto it = context.fieldOptions().formatSpec.begin();
		auto end = context.fieldOptions().formatSpec.end();

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
		
		switch (*it) { // parse early exit and sign spec
			case '}':
				return;

				break;

			case '+':
			case '-':
			case ' ':
				sign = *it;
				++it;

				break;
		}

		if (*it == '#') {
			alternateForm = true;
			++it;
		}

		if (*it == '0') {
			leadingZeros = true;
			++it;
		}

		// don't check the results here because it's optional anyways

		// parse minimum width

		switch (*it) {
			case '{': {
				std::size_t index = context.fieldOptions().fieldIndex;
				
				if (*++it != '}') fromChars(it, end, index);

				context.arg(index).visit([this, &index](const auto& value) {
					using arg_type = std::remove_cvref_t<decltype(value)>;

					if constexpr (std::is_integral_v<arg_type>) width = value;
				});

				break;
			}

			default: 
				if (auto fcRes = fromChars(it, end, width); *fcRes.ptr == '.') {
					fcRes = fromChars(fcRes.ptr + 1, end, precision);

					it = fcRes.ptr;
				}

				break;
		}
		
		// put the type format into a string view for the formatter to deal with

		typeFormat = BasicStringView<CharTy>(it, end);
	}

	char_type fillChr = ' ';
	char_type align = '<';
	
	char_type sign = '-';
	bool alternateForm = false;
	bool leadingZeros = false;

	std::size_t width = 0;
	std::size_t precision = 6;

	BasicStringView<char_type> typeFormat;
};

} // namespace detail

} // namespace lsd
