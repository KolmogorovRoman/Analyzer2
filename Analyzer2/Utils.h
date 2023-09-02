#pragma once
#include <tuple>

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