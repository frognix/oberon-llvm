#pragma once

#include "node.hpp"

namespace nodes {

struct Statement : Node {};

using StatementPtr = OPtr<Statement>;

using StatementSequence = std::vector<StatementPtr>;

}
