#include "module_loader.hpp"
#include "semantic_context.hpp"

SemanticUnitI* ModuleLoader::load(IOManager& io, std::string name) {
    auto res = io.get_file_manager().get_file(name);
    if (!res) return nullptr;
    auto [path, code] = *res;
    auto messages = *io.get_message_manager().get_container(path);
    fmt::print("{}\n", name);
    std::chrono::time_point<std::chrono::steady_clock> start, end;
    std::chrono::milliseconds parse_duration;
    start = std::chrono::steady_clock::now();
    auto parseResult = parser->parse(*code);
    end = std::chrono::steady_clock::now();
    parse_duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    fmt::print("Module {} parse time: {}ms\n", name, parse_duration.count());
    if (!parseResult) {
        messages->addErr(parseResult.get_err().place, parseResult.get_err().to_string().c_str());
        return nullptr;
    }
    auto moduleTree = parseResult.get_ok();
    auto import_error = false;
    std::vector<std::pair<nodes::Import, ModuleTablePtr>> imports;
    for (auto& import : moduleTree.imports) {
        auto res = load(io, import.real_name.to_string());
        if (!res) {
            import_error = true;
            continue;
        }
        std::pair<nodes::Import, ModuleTablePtr> pair(import, static_cast<const ModuleTable*>(res));
        imports.push_back(pair);
    }
    if (import_error) return nullptr;
    auto moduleRes = ModuleTable::parse(moduleTree.name, imports, moduleTree.declarations, moduleTree.body, *messages);
    if (!moduleRes) return nullptr;
    auto module = std::move(*moduleRes);

    if (!module->analyze_code(*messages)) return nullptr;

    units[moduleTree.name] = std::move(module);
    return units[moduleTree.name].get();
}
