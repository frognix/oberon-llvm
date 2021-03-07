#pragma once

#include "parser.hpp"
#include "module_table.hpp"

#include <chrono>

std::string format_error(CodeStream& code, CodePlace place, std::string error);
void print_error(CodeStream& code, CodePlace place, std::string error);

class ModuleLoader;

using ModuleResult = std::variant<ModuleLoader, std::string>;

class ModuleLoader {
public:
    static ModuleLoader load(std::string name, ParserPtr<nodes::Module> parser, std::string& error);
    ModuleTable const* get_table() const { return m_module.get(); }
private:
    ModuleLoader() {}
    std::unique_ptr<ModuleTable> m_module;
    std::vector<ModuleLoader> m_imports;
};
