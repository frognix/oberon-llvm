#pragma once

#include <tuple>
#include <type_traits>

template <typename> struct is_tuple: std::false_type {};
template <typename ...T> struct is_tuple<std::tuple<T...>> : std::true_type {};

//Концепт проверяющий то что тип является специализацией
//шаблона std::tuple
template <class T>
concept isTuple = is_tuple<std::remove_cvref_t<T>>::value;

//Вспомогательные алиасы для определения типа кортежа
template <isTuple Type, size_t... Is>
using select_many = std::tuple<std::tuple_element_t<Is, Type>...>;

template <isTuple Type, size_t Idx>
using select_one = std::tuple_element_t<Idx, Type>;

//Определение типа кортежа выбранного из кортежа Type
template <isTuple Type, size_t Idx, size_t... Is>
using tuple_select_type = std::conditional_t<sizeof...(Is) == 0, select_one<Type, Idx>, select_many<Type, Idx, Is...>>;

//Функция для создания кортежа из другого кортежа по индексам
//Пример
//
//std::tuple<float, std::string, int> tuple(42, "Hello", 3.14);
//
//std::tuple<float, int> parted_tuple = tuple_select<0,2>(tuple);
//
template <size_t... Is, isTuple Type>
inline constexpr auto tuple_select(Type&& tuple) noexcept {
    if constexpr (sizeof...(Is) == 1) {
        return std::get<Is...>(std::forward<Type>(tuple));
    } else {
        return std::tuple(std::get<Is>(std::forward<Type>(tuple))...);
    }
}
