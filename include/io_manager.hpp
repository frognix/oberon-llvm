#pragma once
#include "message_container.hpp"
#include "file_manager.hpp"
#include <filesystem>
#include <optional>
#include <string_view>

class IOManager {
public:
    IOManager() noexcept;
    std::optional<std::pair<CodeStream*,MessageContainer>> get_code_file(fs::path) noexcept;
    void log(std::string_view, std::string_view) noexcept;
private:
    FileManager file_manager;
};
