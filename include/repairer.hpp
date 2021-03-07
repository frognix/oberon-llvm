#pragma once

#include <optional>
#include <stdexcept>

template <class T, class Data, class Err, auto func>
requires std::same_as<typename std::invoke_result_t<decltype(func), T&, const Data&>, std::optional<Err>>
class Repairer {
public:
    Repairer() : value() {}
    template <class... Args>
    Repairer(Args... args) : value(args...) {}
    std::optional<Err> repair(const Data& data) const noexcept {
        if (correct) return {};
        auto err = func(value, data);
        if (!err) correct = true;
        return err;
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
        #warning "This function incredibly unsafe"
        return value;
    }
private:
    mutable bool correct = false;
    mutable T value;
};
