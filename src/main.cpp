#include <iostream>

#include "parser.hpp"

int main(int argc, const char* argv[]) {
    if (argc < 2) {
        std::cout << "No file was provided!" << '\n';
        return 1;
    }

    std::cout << "Transpiling \"" << argv[1] << "\"...\n\n";

    Parser parser{argv[1]};
    auto response = parser.parse();
    if (!response.success) {
        std::cout << response.errorReason << '\n';
        return 1;
    }
    std::cout << parser.getAssembly() << '\n';

    return 0;
}
