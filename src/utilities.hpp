#pragma once

#include <sstream>
#include <string>
#include <vector>

std::vector<std::string> splitString(const std::string& input, char delimiter = ' ') {
    std::stringstream ss{input};
    std::vector<std::string> out;
    std::string token;
    while (std::getline(ss, token, delimiter))
        out.push_back(token);
    return out;
}
