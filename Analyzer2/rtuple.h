#pragma once
#include <functional>
using namespace std;

template<class... T>
struct rtuple;

template<>
struct rtuple<>
{
	template<class fRes, class... fArgs>
	fRes pass(function<fRes(fArgs...)> func)
	{
		return call(func);
	}
	template<class fRes, class... fArgs, class... Args>
	fRes call(function<fRes(fArgs...)> func, Args... args)
	{
		return func(args...);
	}	
	template<indirectly_readable it_begin_t>
	void fill(it_begin_t it_begin)
	{}
};

template<class H, class... T>
struct rtuple<H, T...>
{
	H head;
	rtuple<T...> tail;
	rtuple()
	{}
	rtuple(H head, T... tail):
		head(head),
		tail(tail...)
	{}
	template<class fRes, class... fArgs>
	fRes pass(function<fRes(fArgs...)> func)
	{
		return call(func);
	}
	template<class Func>
	auto pass(Func* func)
	{
		return call(function<Func>(func));
	}
	template<class fRes, class... fArgs, class... Args>
	fRes call(function<fRes(fArgs...)> func, Args... args)
	{
		return tail.call(func, args..., head);
	}
	template<indirectly_readable it_begin_t>
	void fill(it_begin_t it_begin)
	{
		head = static_cast<H>(*it_begin);
		tail.fill(std::next(it_begin));
	}
};