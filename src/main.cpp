#include "parser.hpp"
#include "module_table.hpp"

#include <chrono>

inline void writeHelp(std::ostream& stream) {
    stream << "Usage: oberon INFILE [OUTFILE]" << std::endl << "With no OUTFILE write to standard output" << std::endl;
}

void format_error(CodeStream& code, CodePlace place, std::string error) {
    auto column = place.column;
    auto length = code.line_length(place.line);
    int i = int(length) - column - 2;
    fmt::print("{} {}:\n", format_red("Error on"), place);
    fmt::print("    {}\n", error);
    auto line = code.get_line(place.line);
    fmt::print("{}\n", std::string(line.size(), '-'));
    fmt::print("{}", line);
    fmt::print("{}{}{}\n", std::string(column - 3, ' '), format_red("***^***"), std::string(i >= 0 ? i : 0, ' '));
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

    auto parser = get_parser();

    start = std::chrono::steady_clock::now();
    auto res = parser->parse(code);
    end = std::chrono::steady_clock::now();
    parse_duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    if (res) {
        fmt::print("{}\n", res.get_ok().to_string());
        ModuleTable table(res.get_ok().name, res.get_ok().body);
        table.add_imports(res.get_ok().imports);
        auto semantic = table.parse(res.get_ok().declarations);
        fmt::print("Module table\n{}", table.to_string());
        if (semantic) {
            format_error(code, semantic->get_place(), semantic->get_string());
        }
    } else {
        auto error = res.get_err();
        format_error(code, error.place, error.to_string());
    }
    fmt::print("{}\n", std::string(30, '-'));
    fmt::print("Parse time: {}ms\n", parse_duration.count());
    return 0;
}
