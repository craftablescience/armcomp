#include <cstdlib>
#include <iostream>

#include "parser.hpp"

std::string replaceExtension(const std::string& filename, const std::string& ext) {
    return filename.substr(0, filename.find_last_of('.')) + "." + ext;
}

int main(int argc, const char* argv[]) {
    if (argc < 2) {
        std::cout << "No file was provided!" << '\n';
        return 1;
    }

    std::cout << "Transpiling \"" << argv[1] << "\"...\n";
    Parser parser{argv[1]};
    auto response = parser.parse();
    if (!response.success) {
        std::cout << response.errorReason << '\n';
        return 1;
    }

    std::string outFile = replaceExtension(argv[1], "s");
    if (outFile == argv[1])
        outFile = replaceExtension(argv[1], "compiled.s");
    std::cout << "Saving to \"" << outFile << "\"\n";
    std::fstream out{outFile, std::ios::out};
    std::string assembly = parser.getAssembly();
    out.write(assembly.c_str(), static_cast<int64_t>(assembly.length()));
    out.close();

    std::cout << "Running in simulator...\n\n";
    std::string pycall = "python armsim_runner.py \"" + outFile + '\"';
    return system(pycall.c_str());
}
