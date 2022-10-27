#pragma once

#include <cstdint>
#include <fstream>
#include <string>
#include <unordered_map>
#include <vector>
#include <stack>

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
    [[nodiscard]] std::string getProcedureBlock() const;
    [[nodiscard]] static std::string getDataBlock();
    [[nodiscard]] std::string getAssembly() const;
private:
    std::fstream file;
    FileWriter main;
    FileWriter procedures;

    bool insideASM = false;
    bool insideProcedure = false;

    [[nodiscard]] inline FileWriter& activeCode() {
        return this->insideProcedure ? this->procedures : this->main;
    }

    static inline std::vector<std::string> strings;
    static inline std::stack<std::vector<std::string>> variables;

    [[nodiscard]] static bool parseLogicalOperator(std::string& op);
    [[nodiscard]] static bool parseValue(std::string& value);
    [[nodiscard]] static bool parseStringLiteral(std::string& literal);
};
