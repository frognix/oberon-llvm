#include "module_loader.hpp"
#include "semantic_context.hpp"

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
    MessageManager mm{};
    if (!loader.m_module->add_imports(mm, module.imports)) {
        for (auto msg : mm.get_messages()) {
            error += format_error(code, msg.place, msg.text);
        }
        return loader;
    }
    for (auto& import : loader.m_imports) {
        auto res = loader.m_module->set_module(mm, import.m_module.get());
        if (!res) {
            for (auto msg : mm.get_messages()) {
                error += format_error(code, msg.place, msg.text);
            }
            return loader;
        }
    }
    start = std::chrono::steady_clock::now();
    auto semRes = loader.m_module->parse(module.declarations, mm);
    end = std::chrono::steady_clock::now();
    parse_duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    fmt::print("Module {} semantic check time: {}ms\n", name, parse_duration.count());
    if (!semRes) {
        for (auto msg : mm.get_messages()) {
            error += format_error(code, msg.place, msg.text);
        }
        return loader;
    }
    return loader;
}
