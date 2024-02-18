#pragma once
#include <tuple>
#include <string>
#include <regex>

template < class T, class... Ts >
T& head(std::tuple<T, Ts...>& t)
{
	return  std::get<0>(t);
}

template < std::size_t... Ns, class T, class... Ts >
std::tuple<Ts...>& tail_impl(std::index_sequence<Ns...>, std::tuple<T, Ts...>& t)
{
	return std::tuple<Ts...>(std::get<Ns + 1u>(t)...);
}

template < class T, class... Ts >
std::tuple<Ts...>& tail(std::tuple<T, Ts...>& t)
{
	return  tail_impl(std::make_index_sequence<sizeof...(Ts) - 1u>(), t);
}

std::string regex_escape(const std::string& s);


template<class>
struct function_traits;

template<class R, class C, class... Args>
struct function_traits<R(C::*)(Args...) const>
{
    using type = R(Args...);
    using tuple = std::tuple<Args...>;
};

template<class R, class C, class... Args>
struct function_traits<R(C::*)(Args...)>
{
    using type = R(Args...);
    using tuple = std::tuple<Args...>;
};

//template<class Fn>
//Callable(void*, Fn) -> Callable<
//    typename Get_fn_type<decltype(&Fn::operator())>::type>;
//
//template<class Fn>
//Callable(void*, Fn*) -> Callable<Fn>;