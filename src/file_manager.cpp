#include "io_manager.hpp"

std::optional<std::pair<fs::path, CodeStream*>> FileManager::get_file(std::string path) {
    path = "./" + path;
    auto res = files.find(path);
    if (res != files.end()) return {{path, &res->second}};
    CodeStream file(path + ".Mod");
    if (!file.open()) {
        io_manager->add_top_level_error(fmt::format("File {}.Mod not found"));
        return std::nullopt;
    }
    auto [it, success] = files.insert({path, std::move(file)});
    return {{it->first, &it->second}};
}

bool FileManager::is_open(fs::path file) {
    return files.contains(file);
}
