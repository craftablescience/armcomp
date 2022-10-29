#pragma once

#include <cstdint>
#include <fstream>
#include <string>
#include <unordered_map>
#include <vector>
#include <stack>
#include <unordered_map>

#include "filewriter.hpp"
#include "prelude.hpp"

enum ValueType {
    VALUE_ERROR                = 0,
    VALUE_NUMBER               = 1,
    VALUE_UNDEFINED_IDENTIFIER = 2,
    VALUE_VARIABLE             = 3,
    VALUE_FUNCTION             = 4,
};

class Parser {
public:
    explicit Parser(const std::string& filepath);
    ~Parser();
    [[nodiscard]] std::string parse();
    [[nodiscard]] std::string getCodeBlock() const;
    [[nodiscard]] std::string getProcedureBlock() const;
    [[nodiscard]] std::string getDataBlock() const;
    [[nodiscard]] std::string getAssembly() const;
private:
    std::fstream file;
    FileWriter main;
    FileWriter procedures;

    static bool preprocessLine(std::string& line);
    [[nodiscard]] bool getFileContents(std::vector<std::string>& unparsedLines);

    bool insideASM = false;
    bool insideProcedure = false;

    [[nodiscard]] inline FileWriter& activeCode() {
        return this->insideProcedure ? this->procedures : this->main;
    }

    std::vector<std::string> strings;
    std::stack<std::vector<std::string>> variables;
    std::unordered_map<std::string, std::vector<std::string>> functions;

    [[nodiscard]] inline bool hasVariable(const std::string& varName) const {
        return std::find(this->variables.top().begin(), this->variables.top().end(), varName) != this->variables.top().end();
    }

    [[nodiscard]] static bool isValidIdentifier(const std::string& value);

    void pushVariableStack();
    void popVariableStack();

    [[nodiscard]] ValueType getValueType(const std::string& value);
    ValueType parseValue(std::string& value);

    [[nodiscard]] static bool parseMathOperator(std::string& op);
    [[nodiscard]] static bool parseLogicalOperator(std::string& op);
    [[nodiscard]] static bool parseStringLiteral(std::string& literal);
};
