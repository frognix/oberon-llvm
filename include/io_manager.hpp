#pragma once
#include "message_manager.hpp"
#include "file_manager.hpp"

class IOManager {
public:
    IOManager() noexcept;
    MessageManager& get_message_manager();
    FileManager& get_file_manager();
    void add_top_level_error(std::string);
    void write_errors() const;
private:
    FileManager file_manager;
    MessageManager message_manager;
    std::vector<std::string> top_level_errors;
};
