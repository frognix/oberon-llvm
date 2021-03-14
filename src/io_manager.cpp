#include "io_manager.hpp"
#include <iostream>

IOManager::IOManager() noexcept : file_manager(this), message_manager(this, &file_manager) {}

MessageManager& IOManager::get_message_manager() {
    return message_manager;
}

FileManager& IOManager::get_file_manager() {
    return file_manager;
}

void IOManager::add_top_level_error(std::string err) {
    top_level_errors.push_back(err);
}

void IOManager::write_errors() const {
    for (auto err : top_level_errors) {
        std::cerr << fmt::format("IO: {}\n", err);
    }
    message_manager.write_errors(std::cerr);
}
