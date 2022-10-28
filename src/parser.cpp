#include "parser.hpp"

#include <algorithm>
#include <cctype>
#include <stack>

#include "utilities.hpp"

#define ASM_IF_LABEL_PREFIX "_if"
#define ASM_WHILE_LABEL_PREFIX "_while"
#define ASM_STRING_PREFIX "_str"
#define ASM_PROCEDURE_END_LABEL "_proc_end"
#define ASM_REGISTER_MATH_HELPER "x9"
#define ASM_REGISTER_RETURN_VALUE "x10"
// There is a predefined return variable "_"
#define ASM_REGISTER_OFFSET 10
#define ASM_REGISTER_TRUE_OFFSET 11

Parser::Parser(const std::string& filepath) {
    this->file.open(filepath, std::ios::in);
}

Parser::~Parser() {
    this->file.close();
}

std::string Parser::parse() {
    if (!this->file.is_open())
        return "Could not open file!";

    pushVariableStack();

    this->main << ".text" << ".global _start" << "_start:";
    this->main.indent();
    this->procedures.indent();
    this->procedures << "b ." ASM_PROCEDURE_END_LABEL;

    uint16_t hardcodedLabels = 0;
    std::stack<std::string> endings;

    int callDepth = 0;

    std::string line;
    while (std::getline(this->file, line)) {
        while (!line.empty() && line.starts_with(' '))
            line = line.substr(1);

        // comments must be on their own lines
        if (line.starts_with("//"))
            continue;

        auto lines = splitString(line);
        if (lines.empty())
            continue;

        if (this->insideASM && lines[0] != "end") {
            // replace ${var} with the proper register
            for (int i = 0; i < this->variables.top().size(); i++) {
                std::string varToReplace = "${" + this->variables.top().at(i) + "}";
                replaceSubstring(line, varToReplace, "x" + std::to_string(i + ASM_REGISTER_OFFSET));
            }
            this->activeCode() << line;
            continue;
        }

        if (lines[0] == "if") {
            std::string value1 = lines[1], op = lines[2], value2 = lines[3];
            if (lines.size() != 4 || !parseValue(value1) || !parseLogicalOperator(op) || !parseValue(value2))
                return "Invalid syntax for if call: \"" + line + '\"';

            std::string branch = op;
            std::string label = "." ASM_IF_LABEL_PREFIX + std::to_string(hardcodedLabels++);
            branch += ' ' + label;
            this->activeCode() << "cmp " + value1 + ", " + value2 << branch;
            endings.push(label + ':');

            callDepth++;

        } else if (lines[0] == "while") {
            std::string value1 = lines[1], op = lines[2], value2 = lines[3];
            if (lines.size() != 4 || !parseValue(value1) || !parseLogicalOperator(op) || !parseValue(value2))
                return "Invalid syntax for while call: \"" + line + '\"';

            this->activeCode().dedent();
            this->activeCode() << "." ASM_WHILE_LABEL_PREFIX + std::to_string(hardcodedLabels++) + ':';
            this->activeCode().indent();
            std::string branch = op;
            std::string labelEnd = "." ASM_WHILE_LABEL_PREFIX + std::to_string(hardcodedLabels++);
            branch += ' ' + labelEnd;
            this->activeCode() << "cmp " + value1 + ", " + value2 << branch;
            endings.push("b ." ASM_WHILE_LABEL_PREFIX + std::to_string(hardcodedLabels - 2) + '\n' + labelEnd + ':');

            callDepth++;

        } else if (lines[0] == "func") {
            if (lines.size() < 2 || lines.size() >= 10)
                return "Invalid syntax for func call: \"" + line + '\"';
            if (this->insideProcedure)
                return "Cannot have functions inside functions! (\"" + line + "\")";
            this->insideProcedure = true;

            this->activeCode().dedent();
            this->activeCode() << lines[1] + ':';
            this->activeCode().indent();
            this->activeCode() << "str lr, [sp, #-0x10]!";
            endings.push("ldr lr, [sp], #0x10\n\tret");
            pushVariableStack();

            // Add expected arguments, and copy them to the appropriate registers
            std::vector<std::string> parameters;
            for (int i = 2; i < lines.size(); i++) {
                parameters.push_back(lines[i]);
                this->variables.top().push_back(lines[i]);
                // i-2: x0 when i is 0, etc.
                this->activeCode() << "mov x" + std::to_string(i - 2 + ASM_REGISTER_TRUE_OFFSET) + ", x" + std::to_string(i-2);
            }
            this->functions[lines[1]] = parameters;

            callDepth++;

        } else if (lines[0] == "return") {
            if (lines.size() > 2)
                return "Invalid syntax for return call: \"" + line + '\"';

            if (lines.size() == 2) {
                std::string value = lines[1];
                if (!parseValue(value))
                    return "Invalid syntax for return call: \"" + line + '\"';

                this->activeCode() << "mov " ASM_REGISTER_RETURN_VALUE ", " + value;
            } else {
                this->activeCode() << "mov " ASM_REGISTER_RETURN_VALUE ", #0";
            }
            this->activeCode() << "ldr lr, [sp], #0x10" << "ret";

        } else if (lines[0] == "asm") {
            if (lines.size() > 1)
                return "Invalid syntax for asm call: \"" + line + '\"';
            this->insideASM = true;
            // no need to bump callDepth here

        } else if (lines[0] == "end") {
            if (this->insideASM) {
                this->insideASM = false;
            } else {
                if (endings.top().starts_with('.'))
                    this->activeCode().dedent();
                this->activeCode() << endings.top();
                if (endings.top().starts_with('.'))
                    this->activeCode().indent();
                // Only end function if we are actually ending the function
                if (this->insideProcedure && callDepth == 1) {
                    this->insideProcedure = false;
                    popVariableStack();
                }
                endings.pop();

                callDepth--;
            }

        } else if (lines[0] == "let") {
            if (lines.size() < 4 || lines.size() > 4 || lines[2] != "=" || hasVariable(lines[1]))
                return "Invalid syntax for let call: \"" + line + '\"';
            std::string value = lines[3];
            if (!parseValue(value))
                return "Invalid syntax: \"" + line + '\"';

            this->activeCode() << "mov x" + std::to_string(this->variables.top().size() + ASM_REGISTER_OFFSET) + ", " + value;
            this->variables.top().push_back(lines[1]);

        } else if (lines[0] == "label") {
            if (lines.size() < 2)
                return "Invalid syntax for label: \"" + line + '\"';

            this->activeCode().dedent();
            this->activeCode() << "." + lines[1] + ":";
            this->activeCode().indent();

        } else if (lines[0] == "goto") {
            if (lines.size() < 2)
                return "Invalid syntax for goto: \"" + line + '\"';

            this->activeCode() << "b ." + lines[1];

        } else if (lines[0] == "print") {
            if (lines.size() < 2)
                return "Invalid syntax for print: \"" + line + '\"';

            this->activeCode() << "mov x0, #1";
            auto str = std::string{ASM_STRING_PREFIX} + std::to_string(this->strings.size());
            this->activeCode() << "ldr x1, =" + str << "ldr x2, =" + str + "_len";
            this->activeCode() << "mov x8, 0x40" << "svc 0";
            std::string literal = line.substr(6);
            if (!parseStringLiteral(literal))
                return "Encountered invalid literal: " + literal;
            this->strings.push_back(literal);

        } else if (lines[0] == "println") {
            if (lines.size() < 2)
                return "Invalid syntax for println: \"" + line + '\"';

            this->activeCode() << "mov x0, #1";
            auto str = std::string{ASM_STRING_PREFIX} + std::to_string(this->strings.size());
            this->activeCode() << "ldr x1, =" + str << "ldr x2, =" + str + "_len";
            this->activeCode() << "mov x8, 0x40" << "svc 0";
            std::string literal = line.substr(8);
            if (!parseStringLiteral(literal))
                return "Encountered invalid literal: " + literal;
            this->strings.push_back(literal + "\\n");

        } else if (lines[0] == "exit") {
            if (lines.size() > 2)
                return "Invalid syntax for exit: \"" + line + '\"';

            if (lines.size() > 1) {
                std::string value = lines[1];
                if (!parseValue(value))
                    return "Invalid syntax: \"" + line + '\"';
                this->activeCode() << "mov x0, " + value;
            }
            this->activeCode() << "mov x8, #93" << "svc 0";

        } else if (const auto type = getValueType(lines[0]); type != VALUE_ERROR) {
            switch (type) {
                case VALUE_VARIABLE: {
                    if (lines.size() == 3) {
                        std::string op;
                        if (lines[1] == "=") {
                            op = "mov";
                        } else {
                            op = lines[1].substr(0, 1);
                            if (!parseMathOperator(op))
                                return "Invalid syntax: \"" + line + '\"';
                        }

                        std::string var = lines[0], value = lines[2];
                        if (!parseValue(var) || !parseValue(value))
                            return "Invalid syntax: \"" + line + '\"';

                        this->activeCode() << op + ' ' + var + ", " + (lines[1] != "=" ? var + ", " : "") + value;
                    } else if (lines.size() == 5 && lines[1] == "=") {
                        std::string op = lines[3];
                        if (!parseMathOperator(op))
                            return "Invalid syntax: \"" + line + '\"';

                        std::string var = lines[0], value1 = lines[2], value2 = lines[4];
                        if (!parseValue(var) || !parseValue(value1) || !parseValue(value2))
                            return "Invalid syntax: \"" + line + '\"';
                        // todo: this can be extended to larger equations
                        this->activeCode() << op + " " ASM_REGISTER_MATH_HELPER ", " + value1 + ", " + value2;
                        this->activeCode() << "mov " + var + ", " ASM_REGISTER_MATH_HELPER;
                    } else {
                        return "Invalid syntax: \"" + line + '\"';
                    }
                    break;
                }

                case VALUE_FUNCTION: {
                    if (!this->functions.contains(lines[0]))
                        return "Function is not defined: \"" + line + '\"';
                    if (this->functions.at(lines[0]).size() != lines.size() - 1)
                        return "Function has a different number of parameters: \"" + line + '\"';

                    const auto& vars = this->variables.top();

                    // Copy all arguments to x0-x7 (which are copied to usable registers inside the func)
                    for (int i = 1; i < lines.size(); i++) {
                        std::string value = lines[i];
                        const auto valType = parseValue(value);
                        if (valType == VALUE_NUMBER) {
                            this->activeCode() << "mov x" + std::to_string(i-1) + ", " + value;
                        } else if (valType == VALUE_VARIABLE) {
                            for (const auto& var : vars) {
                                if (var == lines[i]) {
                                    this->activeCode() << "mov x" + std::to_string(i-1) + ", " + value;
                                }
                            }
                        } else {
                            return "Invalid syntax for function call: \"" + line + '\"';
                        }
                    }

                    for (int i = ASM_REGISTER_TRUE_OFFSET; i < vars.size() + ASM_REGISTER_OFFSET; i++) {
                        // this is a waste of space... too bad!
                        this->activeCode() << "str x" + std::to_string(i) + ", [sp, #-0x10]!";
                    }
                    this->activeCode() << "bl " + lines[0];
                    for (int i = static_cast<int>(vars.size()) + ASM_REGISTER_OFFSET - 1; i >= ASM_REGISTER_TRUE_OFFSET; i--) {
                        this->activeCode() << "ldr x" + std::to_string(i) + ", [sp], #0x10";
                    }
                    break;
                }

                default:
                    break;
            }

        } else {
            return "Encountered unexpected operation! Found in line that reads \"" + line + "\"";
        }
    }

    this->main.setIndent(0);
    this->procedures.setIndent(0);
    this->procedures << "." ASM_PROCEDURE_END_LABEL ":";

    popVariableStack();

    return "";
}

std::string Parser::getAssembly() const {
    return this->getCodeBlock() + this->getProcedureBlock() + this->getDataBlock();
}

std::string Parser::getCodeBlock() const {
    return this->main.getContents();
}

std::string Parser::getProcedureBlock() const {
    return this->procedures.getContents();
}

std::string Parser::getDataBlock() const {
    FileWriter out;
    out << ".data";
    int strNum = 0;
    for (const auto& str : this->strings) {
        out << ASM_STRING_PREFIX + std::to_string(strNum) + ": .asciz \"" + str + '\"';
        out << ASM_STRING_PREFIX + std::to_string(strNum) + "_len = .-" ASM_STRING_PREFIX + std::to_string(strNum);
        strNum++;
    }
    return out.getContents();
}

void Parser::pushVariableStack() {
    this->variables.push({"_"});
}

void Parser::popVariableStack() {
    this->variables.pop();
}

static constexpr auto isValidIdentifierChar = [](char c) {
    return std::isalnum(c) || std::isalpha(c) || c == '_';
};

static inline bool isValidIdentifier(const std::string& value) {
    return (std::isalpha(value[0]) || value[0] == '_') && std::all_of(value.begin(), value.end(), isValidIdentifierChar);
}

ValueType Parser::getValueType(const std::string& value) {
    if (value.empty())
        return VALUE_ERROR;
    if (isValidIdentifier(value)) {
        if (hasVariable(value)) {
            return VALUE_VARIABLE;
        } else if (this->functions.contains(value)) {
            return VALUE_FUNCTION;
        }
        return VALUE_UNDEFINED_IDENTIFIER;
    } else if (std::isalnum(value[0])) {
        return VALUE_NUMBER;
    }
    return VALUE_ERROR;
}

ValueType Parser::parseValue(std::string& value) {
    const auto result = this->getValueType(value);
    switch (result) {
        case VALUE_ERROR:
        case VALUE_UNDEFINED_IDENTIFIER:
        case VALUE_FUNCTION:
            break;
        case VALUE_NUMBER:
            value = "#" + value;
            break;
        case VALUE_VARIABLE: {
            const auto find = std::find(this->variables.top().begin(), this->variables.top().end(), value);
            value = "x" + std::to_string(std::distance(this->variables.top().begin(), find) + ASM_REGISTER_OFFSET);
            break;
        }
    }
    return result;
}

bool Parser::parseMathOperator(std::string& op) {
    if (op == "+") {
        op = "add";
    } else if (op == "-") {
        op = "sub";
    } else if (op == "*") {
        op = "mul";
    } else if (op == "/") {
        op = "sdiv";
    } else {
        return false;
    }
    return true;
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

bool Parser::parseStringLiteral(std::string& literal) {
    if (!((literal.starts_with('\"') && literal.ends_with('\"')) || (literal.starts_with('\'') && literal.ends_with('\''))))
        return false;
    literal = literal.substr(1, literal.length() - 2);
    return true;
}
