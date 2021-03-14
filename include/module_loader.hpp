#pragma once

#include "parser.hpp"
#include "module_table_i.hpp"
#include "io_manager.hpp"

#include <chrono>

std::string format_error(CodeStream& code, CodePlace place, std::string error);
void print_error(CodeStream& code, CodePlace place, std::string error);

class ModuleLoader;

using ModuleResult = std::variant<ModuleLoader, std::string>;

class ModuleLoader {
public:
    ModuleLoader(ParserPtr<std::shared_ptr<nodes::IModule>> p) : parser(p) {}
    ModuleTableI* load(IOManager& io, std::string name);
private:
    ParserPtr<std::shared_ptr<nodes::IModule>> parser;
    std::unordered_map<nodes::Ident, std::unique_ptr<ModuleTableI>> units;
};
