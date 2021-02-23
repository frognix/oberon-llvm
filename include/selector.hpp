#pragma once

#include <tuple>

template <size_t Cur, size_t Idx, size_t... Is>
struct sel {
    template <class Type, class... Types>
    requires(Cur != Idx) constexpr auto ecter() {
        return sel<Cur + 1, Idx, Is...>().template ecter<Types...>();
    }
    template <class Type, class... Types>
    requires(Cur == Idx) constexpr auto ecter() {
        if constexpr (sizeof...(Is) == 0) {
            return std::tuple<Type>();
        } else {
            return std::tuple_cat(std::tuple<Type>(), sel<Cur + 1, Is...>().template ecter<Types...>());
        }
    }
    template <class Type, class... Types>
    constexpr auto check() {
        auto res = ecter<Type, Types...>();
        if constexpr (sizeof...(Is) == 0) {
            return std::get<0>(res);
        } else
            return res;
    }
    template <class... Types>
    using type = decltype(sel<Cur, Idx, Is...>().template check<Types...>());

    template <class... Types>
    static constexpr auto select(std::tuple<Types...> args) {
        auto res = std::apply([](auto... args) { return sel<Cur, Idx, Is...>()._select(args...); }, args);
        if constexpr (sizeof...(Is) == 0) {
            return std::get<0>(res);
        } else
            return res;
    }

    template <class Type, class... Types>
    requires(Cur != Idx) constexpr auto _select([[gnu::unused]] Type arg, Types... args) {
        return sel<Cur + 1, Idx, Is...>()._select(args...);
    }
    template <class Type, class... Types>
    requires(Cur == Idx) constexpr auto _select(Type arg, Types... args) {
        if constexpr (sizeof...(Is) == 0) {
            return std::tuple(arg);
        } else {
            return std::tuple_cat(std::tuple(arg), sel<Cur + 1, Is...>()._select(args...));
        }
    }
};

template <size_t... Is>
using selector = sel<0, Is...>;
