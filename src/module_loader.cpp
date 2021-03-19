#include "module_loader.hpp"
#include "module_table.hpp"
#include "section_nodes.hpp"
#include "semantic_context.hpp"

std::unique_ptr<ModuleTableI> load_module(std::shared_ptr<nodes::IModule> module, std::vector<std::pair<nodes::Import, ModuleTablePtr>> imports, MessageContainer& messages) {
    auto module_ptr = module.get();
    if (auto module = dynamic_cast<nodes::Module*>(module_ptr); module) {
        auto modRes = ModuleTable::parse(*module, imports, messages);
        if (!modRes) return {};
        return std::unique_ptr<ModuleTableI>(modRes.release());
    } else if (auto definition = dynamic_cast<nodes::Definition*>(module_ptr); definition) {
        auto defRes = ModuleTable::parse(*definition, imports, messages);
        if (!defRes) return {};
        return std::unique_ptr<ModuleTableI>(defRes.release());
    }
    throw std::runtime_error("Internal error");
}

ModuleTableI* ModuleLoader::load(IOManager& io, std::string module_name) {
    auto res = io.get_code_file(module_name);
    if (!res) return nullptr;
    auto [code, messages] = *res;
    std::chrono::time_point<std::chrono::steady_clock> start, end;
    std::chrono::milliseconds parse_duration;
    start = std::chrono::steady_clock::now();
    auto parseResult = parser->parse(*code);
    end = std::chrono::steady_clock::now();
    parse_duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    io.log("Time", fmt::format("Module {} parse time: {}ms", module_name, parse_duration.count()));
    if (!parseResult) {
        messages.addErr(parseResult.get_err().place, parseResult.get_err().to_string().c_str());
        return nullptr;
    }
    auto moduleTree = parseResult.get_ok();
    auto import_error = false;
    std::vector<std::pair<nodes::Import, ModuleTablePtr>> imports;
    for (auto& import : moduleTree->get_imports()) {
        if (auto unitRes = units.find(import.real_name); unitRes != units.end()) {
            if (unitRes->second.get() == nullptr) {
                messages.addErr(import.name.place, "Can't analyze module {} because of error in module {}", moduleTree->get_name(), import.real_name);
                import_error = true;
            } else {
                std::pair pair(import, static_cast<const ModuleTableI*>(unitRes->second.get()));
                imports.push_back(pair);
            }
        } else {
             auto res = load(io, import.real_name.to_string());
             if (!res) {
                 units[import.real_name] = std::unique_ptr<ModuleTableI>(nullptr);
                 import_error = true;
             } else {
                 std::pair pair(import, static_cast<const ModuleTableI*>(res));
                 imports.push_back(pair);
             }
        }
    }
    if (import_error) return nullptr;
    auto moduleRes = load_module(moduleTree, imports, messages);
    if (!moduleRes.get()) return nullptr;

    if (!moduleRes->analyze_code(messages)) return nullptr;

    units[moduleTree->get_name()] = std::move(moduleRes);
    return units[moduleTree->get_name()].get();
}
