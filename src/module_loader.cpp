#include "module_loader.hpp"
#include "semantic_context.hpp"

SemanticUnitI* ModuleLoader::load(IOManager& io, std::string module_name) {
    auto res = io.get_file_manager().get_file(module_name);
    if (!res) return nullptr;
    auto [path, code] = *res;
    auto messages = *io.get_message_manager().get_container(path);
    std::string str = "";
    for (auto& [name, mod] : units) {
        str += name.to_string() + ",";
    }
    fmt::print("{} : {}\n", module_name, str);
    std::chrono::time_point<std::chrono::steady_clock> start, end;
    std::chrono::milliseconds parse_duration;
    start = std::chrono::steady_clock::now();
    auto parseResult = parser->parse(*code);
    end = std::chrono::steady_clock::now();
    parse_duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    fmt::print("Module {} parse time: {}ms\n", module_name, parse_duration.count());
    if (!parseResult) {
        messages->addErr(parseResult.get_err().place, parseResult.get_err().to_string().c_str());
        return nullptr;
    }
    auto moduleTree = parseResult.get_ok();
    auto import_error = false;
    std::vector<std::pair<nodes::Import, ModuleTablePtr>> imports;
    for (auto& import : moduleTree.imports) {
        if (auto unitRes = units.find(import.real_name); unitRes != units.end()) {
            if (unitRes->second.get() == nullptr) {
                messages->addErr(import.name.place, "Can't analyze module {} because of error in module {}", moduleTree.name, import.real_name);
                import_error = true;
            } else {
                std::pair pair(import, static_cast<const ModuleTable*>(unitRes->second.get()));
                imports.push_back(pair);
            }
        } else {
             auto res = load(io, import.real_name.to_string());
             if (!res) {
                 units[import.real_name] = std::unique_ptr<SemanticUnitI>(nullptr);
                 import_error = true;
             } else {
                 std::pair pair(import, static_cast<const ModuleTable*>(res));
                 imports.push_back(pair);
             }
        }
    }
    if (import_error) return nullptr;
    auto moduleRes = ModuleTable::parse(moduleTree.name, imports, moduleTree.declarations, moduleTree.body, *messages);
    if (!moduleRes) return nullptr;
    auto module = std::move(*moduleRes);

    if (!module->analyze_code(*messages)) return nullptr;

    units[moduleTree.name] = std::move(module);
    return units[moduleTree.name].get();
}
