#pragma once

#include "plib/code_stream.hpp"
#include <filesystem>
#include <map>

namespace fs = std::filesystem;

class IOManager;

class FileManager {
public:
    FileManager(IOManager* io) : io_manager(io) {}
    std::optional<std::pair<fs::path, CodeStream*>> get_file(std::string path);
    bool is_open(fs::path file);
private:
    IOManager* io_manager;
    std::map<fs::path, CodeStream> files;
};
