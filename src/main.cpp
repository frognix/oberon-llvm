#include "oberon_07_parser.hpp"

#include <chrono>

inline void writeHelp(std::ostream& stream) {
    stream << "Usage: oberon INFILE [OUTFILE]" << std::endl
           << "With no OUTFILE write to standard output" << std::endl;
}

int main(int argc, char* argv[]) {
    std::string_view programName{argv[0]};
    std::vector<std::string_view> args;
    for (int i = 1; i < argc; i++) {
        args.push_back(argv[i]);
    }

    if (args.size() < 1 || args.size() > 1) {
        writeHelp(std::cout);
        return 1;
    } else if (args[0] == "-h" || args[0] == "--help") {
        writeHelp(std::cout);
        return 0;
    }

    std::chrono::time_point<std::chrono::steady_clock> start, end;
    std::chrono::milliseconds parse_duration;

    CodeStream code(args[0].data());
    code.open();

    auto parser = oberon07parser::get_parser();

    start = std::chrono::steady_clock::now();
    auto res = parser->parse(code);
    end = std::chrono::steady_clock::now();
    parse_duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    if (res) {
        fmt::print("{}\n", res.get_ok().to_string());
    } else {
        auto error = res.get_err();
        auto column = error.place.column;
        auto length = code.line_length(error.place.line);
        fmt::print("{}", code.get_line(error.place.line));
        int i = int(length)-column-2;
        fmt::print("{}{}{}\n", std::string(column-3, '-'), format_red("^^^^^"), std::string(i >= 0 ? i : 0, '-'));
        fmt::print("{}\n", res.get_err().to_string());
    }
    fmt::print("{}\n", std::string(30, '-'));
    fmt::print("Parse time: {}ms\n", parse_duration.count());
    return 0;
}
