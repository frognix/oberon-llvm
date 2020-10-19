#include "format.hpp"

std::string format_color(int color, std::string_view str) {
    return fmt::format("\x1B[{}m{}\033[0m", color, str);
}

std::string format_red(std::string_view str) {
    return format_color(Red, str);
}
