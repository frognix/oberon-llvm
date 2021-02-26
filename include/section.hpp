#pragma once

#include "node.hpp"

namespace nodes {

struct Section : Node {};

// using SectionPtr = OPtr<Section>;

template <class Subtype, class... Args>
SectionPtr make_section(Args... args) {
    return make_optr<Section, Subtype>(args...);
}

}
