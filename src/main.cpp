#include "module_loader.hpp"

inline void writeHelp(std::ostream& stream) {
    stream << "Usage: oberon INFILE [OUTFILE]" << std::endl << "With no OUTFILE write to standard output" << std::endl;
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

    auto parser = get_parser();
    std::string error;
    auto loader = ModuleLoader::load(args[0].data(), parser, error);
    if (error.size() > 0) fmt::print("{}", error);
    if (loader.get_table() != nullptr) {
        fmt::print("{}", loader.get_table()->to_string());
    }

    return 0;
}
