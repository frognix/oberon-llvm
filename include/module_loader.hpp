#pragma once

#include "parser.hpp"
#include "module_table.hpp"
#include "io_manager.hpp"

#include <chrono>

std::string format_error(CodeStream& code, CodePlace place, std::string error);
void print_error(CodeStream& code, CodePlace place, std::string error);

class ModuleLoader;

using ModuleResult = std::variant<ModuleLoader, std::string>;

class ModuleLoader {
public:
    ModuleLoader(ParserPtr<nodes::Module> p) : parser(p) {}
    SemanticUnitI* load(IOManager& io, std::string name);
private:
    ParserPtr<nodes::Module> parser;
    std::unordered_map<nodes::Ident, std::unique_ptr<SemanticUnitI>> units;
};
