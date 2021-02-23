#pragma once

#include <fmt/format.h>
#include <fmt/ranges.h>

enum color : int
{
    Black = 30,
    Red = 31,
    Green = 32,
    Yellow = 33,
    Blue = 34,
    Magenta = 35,
    Cyan = 36,
    White = 37
};

std::string format_color(int color, std::string_view str);

std::string format_red(std::string_view str);
