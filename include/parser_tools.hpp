#pragma once

#include <vector>
#include <string_view>
#include <memory>

namespace nodes {

using Real = double;
using Integer = int;

template <class T>
using OPtr = std::shared_ptr<T>;

template <class Out, class T, class... Args>
OPtr<Out> make_optr(Args&&... val) {
    return std::static_pointer_cast<Out>(std::make_shared<T>(val...));
}

}
