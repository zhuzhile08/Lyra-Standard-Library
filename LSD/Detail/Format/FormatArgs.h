/*************************
 * @file FormatArgs.h
 * @author Zhile Zhu (zhuzhile08@gmail.com)
 * 
 * @brief Format argument storage
 * 
 * @date 2024-08-20
 * @copyright Copyright (c) 2024
 *************************/

#pragma once

#include "../../Array.h"
#include "../../UniquePointer.h"
#include "../../String.h"
#include "../../StringView.h"

#include "FormatCore.h"

#include <type_traits>
#include <exception>

namespace lsd {

namespace detail {

// Type erased format argument

template <class Context> class TypeErasedFormatArg {
public:
	using context_type = Context;
	using char_type = Context::char_type;
	using iterator = context_type::iterator;
	using options_type = detail::BasicFieldOptions<char_type>;

	template <class Value> constexpr TypeErasedFormatArg(Value& value) :
		m_value(&value),
		m_format([](const void* v, const context_type& context) {
			Formatter<Value, char_type>().format(*static_cast<Value*>(v), context);
			std::formatter<char, char>();
		}) { }

	void format(const context_type& context) const {
		(*m_format)(m_value, context);
	}
	constexpr explicit operator bool() const noexcept {
		return m_value;
	}

private:
	const void*	m_value { };
	void (*m_format) (const void*, const context_type&);
};

} // namespace detail


// Format argument store

template <class Context> class BasicFormatArg {
private:
	using context_type = Context;
	using char_type = context_type::char_type;
	using iterator = context_type::iterator;
	using handle_type = detail::TypeErasedFormatArg<context_type>;
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

// Empty container mimicking an array when there are no format args

template <class Context> class BasicFormatArgStoreEmptyArray {
public:
	using context_type = Context;
	using format_arg = BasicFormatArg<context_type>;

	consteval BasicFormatArgStoreEmptyArray() = default;

	consteval std::size_t size() const noexcept { 
		return 0;
	}
	consteval format_arg operator[](std::size_t) const noexcept {
		return format_arg();
	}
};


// Empty container mimicking a pointer when there are no format args

template <class Context> class BasicFormatArgsEmptyPointer {
public:
	using context_type = Context;
	using format_arg = BasicFormatArg<context_type>;
};


// Container for format argument storage

template <class Context, class... Args> class BasicFormatArgStore {
public:
	using context_type = Context;
	using format_arg = BasicFormatArg<context_type>;
	using empty_arr = BasicFormatArgStoreEmptyArray<context_type>;

	constexpr BasicFormatArgStore(Args&... args) :
		m_args({ format_arg(args)... }) { }

private:
	[[no_unique_address]] std::conditional_t<
		sizeof...(Args) == 0, 
		empty_arr,
		lsd::Array<format_arg, sizeof...(Args)>> m_args;
	
	template <class> friend class ::lsd::BasicFormatArgs;
};

} // namespace detail


// Create format argument storage

template <class Context = BasicFormatContext<char>, class... Args> constexpr auto makeFormatArgs(Args&... args) {
	return detail::BasicFormatArgStore<Context, Args...>(args...);
}

template <class... Args> constexpr auto makeWFormatArgs(Args&... args) {
	return detail::BasicFormatArgStore<BasicFormatContext<wchar_t>, Args...>(args...);
}


// Format argument container

template <class Context> class BasicFormatArgs {
public:
	using context_type = Context;
	using format_arg = BasicFormatArg<context_type>;
	using empty_ptr = detail::BasicFormatArgsEmptyPointer<context_type>;

	template <class... Args> constexpr BasicFormatArgs(
		const detail::BasicFormatArgStore<context_type, Args...>& store
	) : m_size(sizeof...(Args)) {
		if constexpr (sizeof...(Args) != 0) m_args = store.m_args.begin().get(); // This is usually quite bad because the args would go out of scope, but they won't since these are only going to be in the same scope
	}

	constexpr format_arg get(std::size_t i) const noexcept {
		if (i < m_size) return m_args[i];
		else return format_arg();
	}
	constexpr std::size_t size() const noexcept {
		return m_size;
	}

private:
	std::size_t m_size { };

	const format_arg* m_args { };
};

using FormatArgs = BasicFormatArgs<BasicFormatContext<char>>;
using WFormatArgs = BasicFormatArgs<BasicFormatContext<wchar_t>>;

} // namespace lsd
