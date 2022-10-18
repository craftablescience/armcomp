#include "parser.hpp"

#include <algorithm>
#include <cctype>

#include "utilities.hpp"

#define ASM_STRING_PREFIX "s"
#define ASM_REGISTER_OFFSET 10

Parser::Parser(const std::string& filepath) {
    this->file.open(filepath, std::ios::in);
}

Parser::~Parser() {
    this->file.close();
}

ParseResponse Parser::parse() {
    if (!this->file.is_open())
        return {false, "Could not open file!"};

    this->code << ".text" << ".global _start" << "_start:";
    this->code.indent();

    std::string line;
    while (std::getline(this->file, line)) {
        auto lines = splitString(line);
        if (lines.empty()) continue;

        if (lines[0] == "let") {
            if (lines.size() < 4 || lines[2] != "=" || std::find(variables.begin(), variables.end(), lines[1]) != variables.end())
                return {false, "Invalid syntax for let call: \"" + line + '\"'};
            std::string value = lines[3];
            if (!parseValue(value))
                return {false, "Invalid syntax: \"" + line + '\"'};
            this->code << "mov x" + std::to_string(variables.size() + ASM_REGISTER_OFFSET) + ", " + value;
            variables.push_back(lines[1]);
        } else if (lines[0] == "label") {
            if (lines.size() < 2)
                return {false, "Invalid syntax for label: \"" + line + '\"'};
            this->code.dedent();
            this->code << "." + lines[1] + ":";
            this->code.indent();
        } else if (lines[0] == "goto") {
            if (lines.size() < 2)
                return {false, "Invalid syntax for goto: \"" + line + '\"'};
            this->code << "b ." + lines[1];
        } else if (lines[0] == "print") {
            this->code << "mov x0, #1";
            auto str = std::string{ASM_STRING_PREFIX} + std::to_string(strings.size());
            this->code << "ldr x1, =" + str << "ldr x2, =" + str + "_len";
            this->code << "mov x8, 0x40" << "svc 0";
            std::string literal = line.substr(6);
            if (!parseStringLiteral(literal))
                return {false, "Encountered invalid literal: " + literal};
            strings.push_back(literal);
        } else if (lines[0] == "println") {
            this->code << "mov x0, #1";
            auto str = std::string{ASM_STRING_PREFIX} + std::to_string(strings.size());
            this->code << "ldr x1, =" + str << "ldr x2, =" + str + "_len";
            this->code << "mov x8, 0x40" << "svc 0";
            std::string literal = line.substr(8);
            if (!parseStringLiteral(literal))
                return {false, "Encountered invalid literal: " + literal};
            strings.push_back(literal + "\\n");
        } else if (lines[0] == "exit") {
            if (lines.size() > 1) {
                std::string value = lines[1];
                if (!parseValue(value))
                    return {false, "Invalid syntax: \"" + line + '\"'};
                this->code << "mov x0, " + value;
            }
            this->code << "mov x8, #93" << "svc 0";
            break;
        } else if (auto find = std::find(variables.begin(), variables.end(), lines[0]); find != variables.end()) {
            if (lines.size() == 3) {
                std::string op;
                if (lines[1] == "=") {
                    op = "mov";
                } else if (lines[1] == "+=") {
                    op = "add";
                } else if (lines[1] == "-=") {
                    op = "sub";
                } else if (lines[1] == "*=") {
                    op = "mul";
                } else {
                    return {false, "Invalid syntax: \"" + line + '\"'};
                }
                std::string var = lines[0], value = lines[2];
                if (!parseValue(var) || !parseValue(value))
                    return {false, "Invalid syntax: \"" + line + '\"'};
                this->code << op + ' ' + var + ", " + (!op.starts_with("mov") ? var + ", " : "") + value;
            } else if (lines.size() == 5 && lines[1] == "=") {
                std::string op;
                if (lines[3] == "+") {
                    op = "add";
                } else if (lines[3] == "-") {
                    op = "sub";
                } else if (lines[3] == "*") {
                    op = "mul";
                } else {
                    return {false, "Invalid syntax: \"" + line + '\"'};
                }
                std::string var = lines[0], value1 = lines[2], value2 = lines[4];
                if (!parseValue(var) || !parseValue(value1) || !parseValue(value2))
                    return {false, "Invalid syntax: \"" + line + '\"'};
                // todo: this can be extended to larger equations
                this->code << op + " x9, " + value1 + ", " + value2;
                this->code << "mov " + var + ", x9";
            } else {
                return {false, "Invalid syntax: \"" + line + '\"'};
            }
        } else {
            return {false, "Encountered unexpected operation! Found in line that reads \"" + line + "\""};
        }
    }

    this->code.dedent();
    return {true, ""};
}

std::string Parser::getAssembly() const {
    return this->getCodeBlock() + getDataBlock();
}

std::string Parser::getCodeBlock() const {
    return this->code.getContents();
}

std::string Parser::getDataBlock() {
    FileWriter out;
    out << ".data";
    int strNum = 0;
    for (const auto& str : strings) {
        out << ASM_STRING_PREFIX + std::to_string(strNum) + ": .asciz \"" + str + '\"';
        out << ASM_STRING_PREFIX + std::to_string(strNum) + "_len = .-" ASM_STRING_PREFIX + std::to_string(strNum);
        strNum++;
    }
    return out.getContents();
}

bool Parser::parseValue(std::string& value) {
    // todo: sloppily written
    if (value.empty())
        return false;
    if (std::isalpha(value[0])) {
        // variable
        if (auto find = std::find(variables.begin(), variables.end(), value); find != variables.end()) {
            value = "x" + std::to_string(std::distance(variables.begin(), find) + ASM_REGISTER_OFFSET);
            return true;
        }
        return false;
    } else if (std::all_of(value.begin(), value.end(), std::isalnum)) {
        // number
        value = "#" + value;
        return true;
    }
    return false;
}

bool Parser::parseStringLiteral(std::string& literal) {
    if (!((literal.starts_with('\"') && literal.ends_with('\"')) || (literal.starts_with('\'') && literal.ends_with('\''))))
        return false;
    literal = literal.substr(1, literal.length() - 2);
    return true;
}
