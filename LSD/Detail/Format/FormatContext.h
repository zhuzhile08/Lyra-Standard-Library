/************************
 * @file FormatContext.h
 * @author Zhile Zhu (zhuzhile08@gmail.com)
 * 
 * @brief Format context implementatiokn
 * 
 * @date 2024-11-07
 * @copyright Copyright (c) 2024
 ************************/

#pragma once

#include "FormatCore.h"
#include "FormatArgs.h"

#include "../FromChars/Integral.h"

#include "../../String.h"
#include "../../StringView.h"

#include <type_traits>

namespace lsd {

// formatting context class

template <class CharTy> class BasicFormatContext {
public:
	static_assert(std::is_same_v<CharTy, char> || std::is_same_v<CharTy, wchar_t>, "lsd::BasicFormatContext: Format context only accepts char and wchar_t as valid types for template argument CharTy!");

	using char_type = CharTy;
	
	using iterator = detail::BasicFormatBackInserter<char_type>;
	using view_type = BasicStringView<CharTy>;
	using view_iterator = typename view_type::const_iterator;
	using field_options = detail::BasicFieldOptions<CharTy>;

	using format_args = BasicFormatArgs<BasicFormatContext>;
	using format_arg = format_args::format_arg;

	template <class Ty> using formatter_type = Formatter<Ty, char_type>;

public:
	void format(view_type fmt) {
		for (auto it = fmt.begin(); it < fmt.end() && !m_outputIt.done(); it++) {
			switch (*it) {
				case '{':
					if (m_formatArgs.size() > 0) {
						parseReplacementField(it, fmt);

						if (!m_fieldOptions.isReplacementField) {
							m_outputIt = *it;
							break;
						}

						m_formatArgs.get(m_fieldOptions.argumentIndex).visit([this](const auto& value) { 
							using arg_type = std::remove_cvref_t<decltype(value)>;

							if constexpr (!std::is_same_v<arg_type, std::monostate>) {
								if (m_fieldOptions.hasArrayIndex) {
									if constexpr (isArrayValue<arg_type>) {
										using elem_type = std::remove_reference_t<decltype(std::declval<arg_type>()[0])>;

										formatter_type<elem_type>().format(value[m_fieldOptions.arrayIndex], *this);
									}
								} else formatter_type<arg_type>().format(value, *this);
							}
						});
					}

					break;
				
				case '}':
					m_outputIt = *++it; // the reason why I don't put an extra switch block here to check for syntax is because the string is already checked for validity
					break;

				default:
					m_outputIt = *it;
					break;
			}
		}
	}

	constexpr format_arg arg(std::size_t index) const noexcept {
		return m_formatArgs.get(index);
	}
	constexpr iterator& out() noexcept {
		return m_outputIt;
	}
	constexpr const field_options& fieldOptions() const noexcept {
		return m_fieldOptions;
	}

private:
	constexpr BasicFormatContext(iterator&& outputIt, const format_args& args) : m_outputIt(std::move(outputIt)), m_formatArgs(args) { }
	constexpr BasicFormatContext& operator=(const BasicFormatContext&) = default;
	constexpr BasicFormatContext& operator=(BasicFormatContext&&) = default;

	constexpr void parseReplacementField(view_iterator& it, const view_type& fmt) {
		++m_fieldOptions.fieldIndex;

		char_type* helper { };
		
		switch (*++it) { // since the second character only has a few valid options
			case '{':
				m_fieldOptions.isReplacementField = false;
				--m_fieldOptions.fieldIndex;

				return;

				break;
			case '}':
				return;

				break;

			case ':':
				m_fieldOptions.argumentIndex = m_fieldOptions.fieldIndex;
				++it;

				break;

			default:
				auto fcRes = fromChars(it, fmt.end(), m_fieldOptions.argumentIndex);
				// if (fcRes.ec != std::errc { }) throw FormatError("lsd::BasicFormatContext::format(): Format parameter index not valid!");
				it = fcRes.ptr;

				if (*it == '[') {
					fcRes = fromChars(it + 1, fmt.end(), m_fieldOptions.arrayIndex);
					// if (fcRes.ec != std::errc { }) throw FormatError("lsd::BasicFormatContext::format(): Index into format parameter not valid!");
					m_fieldOptions.hasArrayIndex = true;
					it = fcRes.ptr + 1;
				}

				if (*it == ':') ++it;

				break;
		}

		m_fieldOptions.formatSpec = view_type(it, fmt.begin() + fmt.find('}', it - fmt.begin()));

		std::basic_format_args<BasicFormatContext<char_type>> args;
	}

	iterator m_outputIt;
	format_args m_formatArgs;
	field_options m_fieldOptions { };


	// friend classes

	template <class... FmtArgs> friend String format(FormatString<FmtArgs...>, FmtArgs&&...);
	template <class... FmtArgs> friend WString format(WFormatString<FmtArgs...>, FmtArgs&&...);

	template <class OutputIt, class... FmtArgs> friend OutputIt formatTo(OutputIt, std::size_t, FormatString<FmtArgs...>, FmtArgs&&...);
	template <class OutputIt, class... FmtArgs> friend OutputIt formatTo(OutputIt, std::size_t, WFormatString<FmtArgs...>, FmtArgs&&...);

	template <class... FmtArgs> friend void print(std::FILE*, FormatString<FmtArgs...>, FmtArgs&&...);
	template <class... FmtArgs> friend void println(std::FILE*, FormatString<FmtArgs...>, FmtArgs&&...);
};

using FormatContext = BasicFormatContext<char>;
using WFormatContext = BasicFormatContext<wchar_t>;

using FormatArgs = BasicFormatArgs<FormatContext>;
using WFormatArgs = BasicFormatArgs<WFormatContext>;

}
