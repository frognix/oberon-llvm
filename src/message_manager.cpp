#include "io_manager.hpp"
#include <iostream>

std::optional<MessageContainer*> MessageManager::get_container(fs::path path) {
    auto res = containers.find(path);
    if (res != containers.end()) return &res->second.second;
    if (file_manager->is_open(path)) {
        auto res = file_manager->get_file(path);
        if (!res) return {};
        auto [path, file] = *res;
        containers[path] = std::pair(file, MessageContainer());
        return &containers[path].second;
    } else return std::nullopt;
}

std::string format_error(const char* text, int color, CodeStream* code, CodePlace place, std::string error) {
    if (code == nullptr) return error;
    auto point = code->get_code_point(place);
    auto column = point.column;
    auto length = code->line_length(point.line);
    int i = int(length) - column - 2;
    auto line = code->get_line(place);
    auto spaceCount = column >= 3 ? column - 3 : 0;
    return fmt::format("{} on {}:{}: ", format_color(color, text), code->get_filename(), point) + fmt::format("{}\n", error) +
           fmt::format("{}\n", std::string(line.size(), '-')) + fmt::format("{}", line) +
           fmt::format("{}{}{}\n", std::string(spaceCount, ' '), format_color(color, "~~~^~~~"),
                       std::string(i >= 0 ? i : 0, ' ')) +
           fmt::format("{}\n", std::string(line.size(), '-'));
}

void MessageManager::write_errors(std::ostream& errors) const {
    for (auto& [name, pair] : containers) {
        auto [code, cont] = pair;
        for (auto msg : cont.get_messages()) {
            const char* text;
            int color;
            if (msg.priority == MPriority::ERR) {
                text = "Error";
                color = Red;
            } else {
                text = "Warning";
                color = Yellow;
            }
            errors << format_error(text, color, code, msg.place, msg.text);
        }
    }
}
