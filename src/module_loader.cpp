#include "module_loader.hpp"

std::string format_error(CodeStream& code, CodePlace place, std::string error) {
    auto column = place.column;
    auto length = code.line_length(place.line);
    int i = int(length) - column - 2;
    auto line = code.get_line(place.line);
    return fmt::format("{} {}:\n", format_red("Error on"), place) + fmt::format("    {}\n", error) +
           fmt::format("{}\n", std::string(line.size(), '-')) + fmt::format("{}", line) +
           fmt::format("{}{}{}\n", std::string(column - 3, ' '), format_red("~~~^~~~"),
                       std::string(i >= 0 ? i : 0, ' '));
}

void print_error(CodeStream& code, CodePlace place, std::string error) {
    fmt::print(format_error(code, place, error));
}

ModuleLoader ModuleLoader::load(std::string name, ParserPtr<nodes::Module> parser, std::string& error) {
    auto codename = name + ".Mod";
    fmt::print("{}\n", codename);
    CodeStream code(codename);
    if (!code.open()) {
        error = fmt::format("File for module '{}' not found", name);
        return {};
    }
    std::chrono::time_point<std::chrono::steady_clock> start, end;
    std::chrono::milliseconds parse_duration;
    start = std::chrono::steady_clock::now();
    auto parseResult = parser->parse(code);
    end = std::chrono::steady_clock::now();
    parse_duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    fmt::print("Module {} parse time: {}ms\n", name, parse_duration.count());
    if (!parseResult) {
        error = format_error(code, parseResult.get_err().place, parseResult.get_err().to_string());
        return {};
    }
    auto module = parseResult.get_ok();
    ModuleLoader loader;
    for (auto import : module.imports) {
        auto res = ModuleLoader::load(import.real_name.to_string(), parser, error);
        if (error.size() != 0)
            return {};
        loader.m_imports.push_back(std::move(res));
    }
    loader.m_module = std::make_unique<ModuleTable>(module.name, module.body);
    loader.m_module->add_imports(module.imports);
    for (auto& import : loader.m_imports) {
        auto err = loader.m_module->set_module(import.m_module.get());
        if (err) {
            error = format_error(code, err->get_place(), err->get_string());
            return loader;
        }
    }
    start = std::chrono::steady_clock::now();
    auto semError = loader.m_module->parse(module.declarations);
    end = std::chrono::steady_clock::now();
    parse_duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    fmt::print("Module {} semantic check time: {}ms\n", name, parse_duration.count());
    if (semError) {
        error = format_error(code, semError->get_place(), semError->get_string());
        return loader;
    }
    return loader;
}
