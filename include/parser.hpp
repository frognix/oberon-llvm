#pragma once

#include <variant>
#include <vector>

#include "libparser/parser.hpp"

#include "nodes.hpp"

ParserPtr<std::shared_ptr<nodes::IModule>> get_parsers();
