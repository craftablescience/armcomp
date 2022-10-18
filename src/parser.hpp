#pragma once

#include <cstdint>
#include <fstream>
#include <string>
#include <unordered_map>
#include <vector>

#include "filewriter.hpp"

struct ParseResponse {
    bool success;
    std::string errorReason;
};

class Parser {
public:
    explicit Parser(const std::string& filepath);
    ~Parser();
    [[nodiscard]] ParseResponse parse();
    [[nodiscard]] std::string getCodeBlock() const;
    [[nodiscard]] static std::string getDataBlock();
    [[nodiscard]] std::string getAssembly() const;
private:
    std::fstream file;
    FileWriter code;
    static inline std::vector<std::string> strings;
    static inline std::vector<std::string> variables;

    [[nodiscard]] static bool parseLogicalOperator(std::string& op);
    [[nodiscard]] static bool parseValue(std::string& value);
    [[nodiscard]] static bool parseStringLiteral(std::string& literal);
};
