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
private:
	using char_type = CharType;
	using iterator = Iterator;
	using options_type = detail::BasicFieldOptions<char_type>;

	class BasicValueHandle {
	public:
		constexpr BasicValueHandle(const void* value) : m_value(value) { }

		virtual void format(iterator&, const options_type& options) const = 0;
		virtual constexpr UniquePointer<BasicValueHandle> clone() const = 0;
	
	protected:
		const void*	m_value;
	};

	template <class Ty> class ValueHandle : public BasicValueHandle {
	public:
		using value_type = Ty;
		using formatter = Formatter<value_type, char_type>;

		constexpr ValueHandle(const value_type& value) : BasicValueHandle(&value) { }

		void format(iterator& it, const options_type& options) const {
			formatter::format(*static_cast<value_type*>(m_value), it, options);
		}
		constexpr UniquePointer<BasicValueHandle> clone() const {
			return new ValueHandle(*static_cast<value_type*>(m_value));
		}
	};

public:
	template <class Value> constexpr TypeErasedFormatArg(const Value& value) {
		m_value = new ValueHandle<Value>(value);
	}
	constexpr TypeErasedFormatArg(const TypeErasedFormatArg& other) : m_value(other.m_value->clone()) { }
	constexpr TypeErasedFormatArg(TypeErasedFormatArg&&) = default;

	constexpr TypeErasedFormatArg& operator=(const TypeErasedFormatArg& other) {
		m_value = other.m_value->clone();
	}
	constexpr TypeErasedFormatArg& operator=(TypeErasedFormatArg&&) = default;


	void format(iterator& it, const options_type& options) const {
		m_value->format(it, options);
	}

	constexpr explicit operator bool() const noexcept {
		return m_value;
	}

private:
	UniquePointer<BasicValueHandle> m_value;
};


// type erased 

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
	template <class Value> constexpr BasicFormatArg(Value&& value) noexcept {
		using type = std::remove_cvref_t<Value>;

		if constexpr (std::is_same_v<bool, type> || std::is_same_v<char_type, type> || std::is_same_v<float, type> || std::is_same_v<double, type> || std::is_same_v<long double, type>) 
			m_value = value;
		else if constexpr (std::is_same_v<char, type> || std::is_same_v<wchar_t, char_type>) 
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
		else if constexpr (std::is_void_v<std::remove_pointer_t<type>> || std::is_null_pointer_v<type>) 
			m_value = implicitCast<const void*>(value);
		else 
			m_value = handle_type(value);
	}

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
};


// Container for multiple format arguments

template <class Context, class... Args> class BasicFormatArgs {
public:
	using context_type = Context;
	using format_arg = BasicFormatArg<Context>;

	constexpr BasicFormatArgs(Args&&... args) noexcept {
		
	}

	format_arg& get(std::size_t i) noexcept {
		
	}
private:

};

} // namespace lsd
