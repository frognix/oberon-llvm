#include "message_container.hpp"
#include "plib/format.hpp"

std::string format_error(const char* text, int color, CodeStream& code, CodePlace place, std::string error) {
    auto point = code.get_code_point(place);
    auto column = point.column;
    auto length = code.line_length(point.line);
    int i = int(length) - column - 2;
    auto line = code.get_line(place);
    auto spaceCount = column >= 3 ? column - 3 : 0;
    return fmt::format("{} on {}:{}: ", format_color(color, text), code.get_filename(), point) + fmt::format("{}\n", error) +
           fmt::format("{}\n", std::string(line.size(), '-')) + fmt::format("{}", line) +
           fmt::format("{}{}{}\n", std::string(spaceCount, ' '), format_color(color, "~~~^~~~"),
                       std::string(i >= 0 ? i : 0, ' ')) +
           fmt::format("{}\n", std::string(line.size(), '-'));
}

void MessageContainer::addMessage(Message message) {
    const char* text;
    int color;
    if (message.priority == MPriority::ERR) {
        text = "Error";
        color = Red;
    } else {
        text = "Warning";
        color = Yellow;
    }
    m_stream << format_error(text, color, m_code, message.place, message.text);
}
