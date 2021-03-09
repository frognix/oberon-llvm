#pragma once

#include "message_container.hpp"
#include <filesystem>
#include <map>
#include "file_manager.hpp"

namespace fs = std::filesystem;

class IOManager;

class MessageManager {
public:
    MessageManager(IOManager* io, FileManager* fm) : io_manager(io), file_manager(fm) {}
    std::optional<MessageContainer*> get_container(fs::path path);
    void write_errors(std::ostream&) const;
private:
    IOManager* io_manager;
    FileManager* file_manager;
    std::map<fs::path, std::pair<CodeStream*, MessageContainer>> containers;
};
