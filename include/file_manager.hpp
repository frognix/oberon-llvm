#pragma once

#include "libparser/code_stream.hpp"
#include <filesystem>
#include <map>

namespace fs = std::filesystem;

class IOManager;

class FileManager {
public:
    FileManager() {}
    std::optional<CodeStream*> get_file(std::string path);
    bool is_open(fs::path file);
private:
    std::map<fs::path, CodeStream> files;
};
