/*************************
 * @file Format.h
 * @author Zhile Zhu (zhuzhile08@gmail.com)
 * 
 * @brief Formatting utility and library
 * 
 * @brief Formatting specification: 
 * @brief replacementField  ::= "{" [field][":"format] "}"
 * @brief field             ::= [argumentIndex]["["elementIndex"]"]
 * @brief format            ::= [[fillCharacter]alignMode][sign]["z"]["#"]["0"][fillCount]["."precision][typeFormat]
 * @brief fillCharacter     ::= <any character except '{' and '}'>
 * @brief alignMode         ::= '<' | '>' | '='
 * @brief sign              ::= '+' | '-' | ' '
 * @brief fillCount        	::= <unsigned integer>
 * @brief precision			::= <unsigned integer>
 * @brief typeFormat        ::= <extended formatting arguments, ususally used for data presentation style>
 * 
 * @date 2024-06-18
 * 
 * @copyright Copyright (c) 2024
 *************************/

#pragma once

#include "Iterators.h"
#include "Array.h"
#include "String.h"
#include "StringView.h"
#include "UniquePointer.h"
#include "FunctionPointer.h"
#include "Utility.h"

#include "Detail/FromChars/FromCharsIntegral.h"
#include "Detail/Format/Formatters.h"
#include "Detail/Format/FormatArgs.h"
#include "Detail/Format/FormatCore.h"

#include <cctype>
#include <type_traits>
#include <concepts>

namespace lsd {

// formatting core context class
template <class CharTy> class BasicFormatContext {
public:
	static_assert(std::is_same_v<CharTy, char> || std::is_same_v<CharTy, wchar_t>, "lsd::BasicFormatContext: Format context only accepts char and wchar_t as valid types for template argument CharTy!");

	using char_type = CharTy;
	
	using iterator = detail::BasicFormatBackInserter<char_type>;
	using view_type = BasicStringView<CharTy>;
	using view_iterator = typename view_type::const_iterator;
	using field_options = detail::BasicFieldOptions<CharTy>;

	template <class... Args> using format_args = detail::BasicFormatArgStore<BasicFormatContext, Args...>;

	template <class Ty> using formatter_type = Formatter<Ty, char_type>;

public:
	constexpr BasicFormatContext() = delete;
	constexpr BasicFormatContext& operator=(const BasicFormatContext&) = delete;
	constexpr BasicFormatContext& operator=(BasicFormatContext&&) = delete;

	template <class... Args> constexpr static void format(iterator outputIt, view_type fmt, Args&... args) {
		auto argList = 	format_args<Args...>(args...);
		field_options options { };

		for (auto it = fmt.begin(); it < fmt.end() && !outputIt.done(); it++) {
			switch (*it) {
				case '{':
					if constexpr (sizeof...(Args) > 0) {
						options = parseReplacementField(it, fmt.end(), options.fieldIndex);

						if (!options.isReplacementField) {
							outputIt = *it;
							break;
						}

						if (options.hasArrayIndex) argList.get(options.argumentIndex).visit([&outputIt, &options](auto&& value) { 
							using arg_type = std::remove_cvref_t<decltype(value)>;

							if constexpr (!std::is_same_v<arg_type, std::monostate> && isArrayValue<arg_type>) formatter_type<
									std::remove_reference_t<decltype(
										std::declval<arg_type>()[0]
									)>
								>().format(value[options.arrayIndex], outputIt, options);
						});
						else argList.get(options.argumentIndex).visit([&outputIt, &options](auto&& value) { 
							using arg_type = std::remove_cvref_t<decltype(value)>;

							if constexpr (!std::is_same_v<arg_type, std::monostate>) formatter_type<arg_type>().format(value, outputIt, options);
						});
					}

					break;
				
				case '}':
					outputIt = *++it; // the reason why I don't put an extra switch block here to check for syntax is because the string is already checked for validity
					break;

				default:
					outputIt = *it;
					break;
			}
		}
	}

private:
	constexpr static field_options parseReplacementField(view_iterator& it, view_iterator end, std::size_t prevFieldIndex) {
		field_options fieldOptions;
		fieldOptions.fieldIndex = ++prevFieldIndex;

		char_type* helper { };
		
		switch (*++it) { // since the second character only has a few valid options
			case '{':
				fieldOptions.isReplacementField = false;
				--fieldOptions.fieldIndex;
				return fieldOptions;

				break;
			case '}':
				fieldOptions.useDefaultFormat = true;
				return fieldOptions;

				break;

			case ':':
				fieldOptions.argumentIndex = fieldOptions.fieldIndex;

				break;

			default:
				auto fcRes = fromChars(it, end, fieldOptions.argumentIndex);
				// if (fcRes.ec != std::errc { }) throw FormatError("lsd::BasicFormatContext::format(): Format parameter index not valid!");
				it = fcRes.ptr;

				if (*it == '[') {
					fcRes = fromChars(it + 1, end, fieldOptions.arrayIndex);
					// if (fcRes.ec != std::errc { }) throw FormatError("lsd::BasicFormatContext::format(): Index into format parameter not valid!");
					fieldOptions.hasArrayIndex = true;
					it = fcRes.ptr + 1;
				}

				if (*it == ':') ++it;

				break;
		}

		switch (*it) { // parse early exit end alignment options
			case '}':
				return fieldOptions;

				break;

			default: {
				auto alignFirst = *it;

				switch (*++it) {
					case '<':
					case '>':
					case '=':
						fieldOptions.align = *it;
						fieldOptions.fillChr = alignFirst;
						++it;

						break;

					default:
						switch (alignFirst) {
							case '<':
							case '>':
							case '=':
								fieldOptions.align = alignFirst;
								++it;

								break;
						}
				}

				break;
			}
		}
		
		switch (*it) { // parse early exit and sign options
			case '}':
				return fieldOptions;

				break;

			case '+':
			case '-':
			case ' ':
				fieldOptions.sign = *it;
				++it;

				break;
		}

		if (*it == 'z') {
			fieldOptions.negativeZero = false;
			++it;
		}

		if (*it == '#') {
			fieldOptions.alternateForm = true;
			++it;
		}

		if (*it == '0') {
			fieldOptions.leadingZeros = true;
			++it;
		}

		// don't check the results here because it's optional anyways

		auto fcRes = fromChars(it, end, fieldOptions.fillCount);

		if (*fcRes.ptr == '.') fcRes = fromChars(fcRes.ptr + 1, end, fieldOptions.precision);
		
		// parse the remaining formatting option

		for (it = fcRes.ptr; *it != '}'; it++)
		;

		fieldOptions.typeFormat = view_type(fcRes.ptr, it);

		return fieldOptions;
	}
};

using FormatContext = BasicFormatContext<char>;
using WFormatContext = BasicFormatContext<wchar_t>;


// general formatting functions

template <class... Args> inline String format(FormatString<Args...> fmt, Args&&... args) {
	String out;
	FormatContext::format(
		detail::FormatBackInserter(
			&out,
			[](void* out, const char& v) { static_cast<String*>(out)->pushBack(v); },
			[](void* out) { return false; }
		), 
		fmt.get(), 
		args...
	);
	return out;
}
template <class... Args> inline WString format(WFormatString<Args...> fmt, Args&&... args) {
	WString out;
	FormatContext::format(
		detail::FormatBackInserter(
			&out,
			[](void* out, const wchar_t& v) { static_cast<WString*>(out)->pushBack(v); },
			[](void* out) { return false; }
		), 
		fmt.get(), 
		std::forward<Args>(args)...
	);
	return out;
}


template <class OutputIt, class... Args> inline OutputIt formatTo(OutputIt it, std::size_t n, FormatString<Args...> fmt, Args&&... args) {
	
}
template <class OutputIt, class... Args> inline OutputIt formatTo(OutputIt it, std::size_t n, WFormatString<Args...> fmt, Args&&... args) {
	
}
template <class OutputIt, class... Args> inline OutputIt format_to(OutputIt it, std::size_t n, FormatString<Args...> fmt, Args&&... args) {
	
}
template <class OutputIt, class... Args> inline OutputIt format_to(OutputIt it, std::size_t n, WFormatString<Args...> fmt, Args&&... args) {
	
}


template <class... Args> inline void print(FormatString<Args...> fmt, Args&&... args) {
	print(stdout, fmt, std::forward<Args>(args)...);
}
template <class... Args> inline void print(std::FILE* stream, FormatString<Args...> fmt, Args&&... args) {
	
}
template <class... Args> inline void println(FormatString<Args...> fmt, Args&&... args) {
	println(stdout, fmt, std::forward<Args>(args)...);
}
template <class... Args> inline void println(std::FILE* stream, FormatString<Args...> fmt, Args&&... args) {
	
}

} // namespace lsd
