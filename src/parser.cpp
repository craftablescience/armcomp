#include "parser.hpp"

#include <algorithm>
#include <cctype>
#include <stack>

#include "utilities.hpp"

#define ASM_IF_LABEL_PREFIX "_i"
#define ASM_WHILE_LABEL_PREFIX "_w"
#define ASM_STRING_PREFIX "_s"
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

    uint16_t hardcodedLabels = 0;
    std::stack<std::string> endings;

    std::string line;
    while (std::getline(this->file, line)) {
        while (!line.empty() && line.starts_with(' '))
            line = line.substr(1);

        auto lines = splitString(line);
        if (lines.empty()) continue;

        if (lines[0] == "if") {
            std::string value1 = lines[1], op = lines[2], value2 = lines[3];
            if (lines.size() != 4 || !parseValue(value1) || !parseLogicalOperator(op) || !parseValue(value2))
                return {false, "Invalid syntax for if call: \"" + line + '\"'};
            std::string branch = op;
            std::string label = "." ASM_IF_LABEL_PREFIX + std::to_string(hardcodedLabels++);
            branch += ' ' + label;
            this->code << "cmp " + value1 + ", " + value2 << branch;
            endings.push(label + ':');
        } else if (lines[0] == "while") {
            std::string value1 = lines[1], op = lines[2], value2 = lines[3];
            if (lines.size() != 4 || !parseValue(value1) || !parseLogicalOperator(op) || !parseValue(value2))
                return {false, "Invalid syntax for while call: \"" + line + '\"'};
            this->code.dedent();
            this->code << "." ASM_WHILE_LABEL_PREFIX + std::to_string(hardcodedLabels++) + ':';
            this->code.indent();
            std::string branch = op;
            std::string labelEnd = "." ASM_WHILE_LABEL_PREFIX + std::to_string(hardcodedLabels++);
            branch += ' ' + labelEnd;
            this->code << "cmp " + value1 + ", " + value2 << branch;
            endings.push("b ." ASM_WHILE_LABEL_PREFIX + std::to_string(hardcodedLabels - 2) + '\n' + labelEnd + ':');
        } else if (lines[0] == "end") {
            if (endings.top().starts_with('.'))
                this->code.dedent();
            this->code << endings.top();
            if (endings.top().starts_with('.'))
                this->code.indent();
            endings.pop();
        } else if (lines[0] == "let") {
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

bool Parser::parseLogicalOperator(std::string& op) {
    // remember to invert the condition since we jump to the end if true
    if (op == "==") {
        op = "ne";
    } else if (op == "!=") {
        op = "eq";
    } else if (op == "<") {
        op = "ge";
    } else if (op == ">") {
        op = "le";
    } else if (op == "<=") {
        op = "gt";
    } else if (op == ">=") {
        op = "lt";
    } else {
        return false;
    }
    op = 'b' + op;
    return true;
}

bool Parser::parseValue(std::string& value) {
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
