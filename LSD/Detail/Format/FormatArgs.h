/*************************
 * @file FormatArgs.h
 * @author Zhile Zhu (zhuzhile08@gmail.com)
 * 
 * @brief Format argument storage
 * 
 * @date 2024-08-20
 * 
 * @copyright Copyright (c) 2024
 *************************/

#pragma once

#include "../../UniquePointer.h"
#include "../../String.h"
#include "../../StringView.h"

#include "FormatCore.h"

#include <type_traits>
#include <exception>

namespace lsd {

namespace detail {

// type erased format argument

template <class CharType, class Iterator> class TypeErasedFormatArg {
public:
	using char_type = CharType;
	using iterator = Iterator;
	using options_type = detail::BasicFieldOptions<char_type>;

	template <class Value> constexpr TypeErasedFormatArg(Value& value) :
		m_value(&value),
		m_format([](const void* v, iterator& it, const options_type& options) {
			Formatter<Value, char_type>().format(*static_cast<Value*>(v), it, options);
			std::formatter<char, char>();
		}) { }

	void format(iterator& it, const options_type& options) const {
		(*m_format)(m_value, it, options);
	}
	constexpr explicit operator bool() const noexcept {
		return m_value;
	}

private:
	const void*	m_value { };
	void (*m_format) (const void*, iterator&, const options_type&);
};

} // namespace detail


// format argument store

template <class ContextType> class BasicFormatArg {
private:
	using context_type = ContextType;
	using char_type = context_type::char_type;
	using iterator = context_type::iterator;
	using handle_type = detail::TypeErasedFormatArg<char_type, iterator>;
	using options_type = detail::BasicFieldOptions<char_type>;

	using variant_type = std::variant<
		std::monostate,
		bool,
		char_type,
		int,
		unsigned int,
		long long int,
		unsigned long long int,
		float,
		double,
		long double,
		const char_type*,
		BasicStringView<char_type>,
		const void*,
		handle_type
	>;

public:
	constexpr BasicFormatArg() noexcept : m_value(std::monostate()) { }

	constexpr explicit operator bool() const noexcept {
		return !std::holds_alternative<std::monostate>(m_value);
	}

	template <class Visitor> constexpr decltype(auto) visit(Visitor&& visitor) {
		return std::visit(std::forward<Visitor>(visitor), m_value);
	}
	template <class Value, class Visitor> constexpr Value visit(Visitor&& visitor) {
		return std::visit<Value>(std::forward<Visitor>(visitor), m_value);
	}

private:
	variant_type m_value;

	template <class Value> constexpr BasicFormatArg(Value&& value) noexcept {
		using type = std::remove_cvref_t<Value>;

		if constexpr (std::is_same_v<bool, type> || std::is_same_v<char_type, type> || std::is_same_v<float, type> || std::is_same_v<double, type> || std::is_same_v<long double, type>) 
			m_value = value;
		else if constexpr (std::is_same_v<char, type> && std::is_same_v<wchar_t, char_type>) 
			m_value = implicitCast<wchar_t>(implicitCast<unsigned char>(value));
		else if constexpr (std::is_integral_v<type> && sizeof(type) <= sizeof(int)) 
			m_value = implicitCast<int>(value);
		else if constexpr (std::is_integral_v<type> && std::is_unsigned_v<type> && sizeof(type) <= sizeof(unsigned int)) 
			m_value = implicitCast<unsigned int>(value);
		else if constexpr (std::is_integral_v<type> && sizeof(type) <= sizeof(long long)) 
			m_value = implicitCast<long long>(value);
		else if constexpr (std::is_integral_v<type> && std::is_unsigned_v<type> && sizeof(type) <= sizeof(unsigned long long)) 
			m_value = implicitCast<unsigned long long>(value);
		else if constexpr (std::is_same_v<BasicStringView<char_type>, type> || std::is_same_v<BasicString<char_type>, type>) 
			m_value = BasicStringView<char_type>(value.data(), value.size());
		else if constexpr (std::is_same_v<std::decay_t<type>, char_type*> || std::is_same_v<std::decay_t<type>, const char_type*>) 
			m_value = implicitCast<const char_type*>(value);
		else if constexpr (std::is_pointer_v<std::remove_pointer_t<type>> || std::is_null_pointer_v<type>) 
			m_value = implicitCast<const void*>(value);
		else 
			m_value = handle_type(value);
	}

	template <class, class...> friend class detail::BasicFormatArgStore;
};


namespace detail {

// container for the case then basic format arg store has no arguments

template <class Context> class BasicFormatArgStoreEmptyValue {
public:
	using context_type = Context;
	using format_arg = BasicFormatArg<context_type>;

	constexpr BasicFormatArgStoreEmptyValue() = default;

	constexpr std::size_t size() const noexcept { 
		return 0;
	}
	constexpr format_arg operator[](std::size_t) const noexcept {
		return format_arg();
	}
};


// container for multiple format arguments

template <class Context, class... Args> class BasicFormatArgStore {
public:
	using context_type = Context;
	using format_arg = BasicFormatArg<context_type>;
	using empty_val = BasicFormatArgStoreEmptyValue<context_type>;

	constexpr BasicFormatArgStore(Args&... args) :
		m_args({ format_arg(args)... }) { }

	constexpr format_arg get(std::size_t i) const noexcept {
		if (i < m_args.size()) return m_args[i];
		else return format_arg();
	}
	constexpr std::size_t size() const noexcept {
		return m_args.size();
	}

private:
	[[no_unique_address]] std::conditional_t<
		sizeof...(Args) == 0, 
		empty_val, 
		lsd::Array<format_arg, sizeof...(Args)>> m_args;
};

} // namespace detail

} // namespace lsd
