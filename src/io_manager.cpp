#include "io_manager.hpp"
#include "message_container.hpp"
#include <iostream>
#include <optional>
#include "libparser/format.hpp"

IOManager::IOManager() noexcept {}

std::optional<std::pair<CodeStream*,MessageContainer>> IOManager::get_code_file(fs::path path) noexcept {
    auto file = file_manager.get_file(path);
    if (!file) {
        log("IO", fmt::format("File '{}' not found", path.c_str()));
        return std::nullopt;
    }
    return std::pair{*file, MessageContainer(std::cerr, *file.value())};
}

void IOManager::log(std::string_view type, std::string_view text) noexcept {
    auto header = format_color(Blue, fmt::format("Log({}): ", type));
    std::cerr << fmt::format("{} {}\n", header, text);
}
