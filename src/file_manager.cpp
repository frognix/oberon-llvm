#include "io_manager.hpp"
#include <algorithm>
#include <filesystem>

std::optional<fs::path> find_file(const std::vector<fs::path> dirs, fs::path filename, std::vector<fs::path> extensions) {
    for (auto& path : dirs) {
        auto it = fs::directory_iterator(path);
        fs::directory_iterator it_end;
        auto res = std::find_if(it, it_end, [&filename, &extensions](auto& entry) {
            auto cur_file = entry.path().filename().stem();
            if (cur_file == filename) {
                auto cur_extension = entry.path().filename().extension();
                auto res = std::find(extensions.begin(), extensions.end(), cur_extension);
                if (res != extensions.end()) return true;
            }
            return false;
        });
        if (res != it_end) return res->path();
    }
    return {};
}

std::optional<CodeStream*> FileManager::get_file(std::string path) {
    auto res = files.find(path);
    if (res != files.end()) return {&res->second};
    auto filepath = find_file({"./", "/usr/share/doc/obnc/obncdoc/obnc/"}, path, {".Mod", ".mod", ".def"});
    if (!filepath)
        return std::nullopt;
    CodeStream file(*filepath);
    if (!file.open())
        return std::nullopt;
    auto [it, success] = files.insert({*filepath, std::move(file)});
    return {&it->second};
}

bool FileManager::is_open(fs::path file) {
    return files.contains(file);
}
