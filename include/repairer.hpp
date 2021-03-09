#pragma once

#include "semantic_context.hpp"
#include <optional>
#include <stdexcept>

template <class T, class Data, auto func>
requires std::same_as<typename std::invoke_result_t<decltype(func), T&, Data&>, bool>
class Repairer {
public:
    Repairer() : value() {}
    template <class... Args>
    Repairer(Args... args) : value(args...) {}
    bool repair(Data& data) const noexcept {
        if (correct) return bsuccess;
        auto res = func(value, data);
        if (res) correct = true;
        return res;
    }
    const T& get() const {
        if (!correct) throw std::runtime_error("Attempt to use Repairer::get before repair");
        return value;
    }
    T& get_mut() {
        if (!correct) throw std::runtime_error("Attempt to use Repairer::get before repair");
        return value;
    }
    const T& unsafe_get() const {
        // #warning "This function incredibly unsafe"
        return value;
    }
private:
    mutable bool correct = false;
    mutable T value;
};
