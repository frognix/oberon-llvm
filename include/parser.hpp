#pragma once

#include <variant>
#include <vector>

#include "plib/parser.hpp"

#include "nodes.hpp"

ParserPtr<nodes::Module> get_parser();
