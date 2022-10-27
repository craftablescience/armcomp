#pragma once

#include <string>

class FileWriter {
public:
    FileWriter() = default;
    FileWriter& operator<<(const auto& line) {
        this->writeLine(line);
        return *this;
    }
    void writeLine(const auto& line, bool newline = true) {
        for (int i = 0; i < this->indentLevel; i++) {
            this->contents += '\t';
        }
        this->contents += line + std::string{newline ? "\n" : ""};
    }
    [[nodiscard]] const std::string& getContents() const;
    [[nodiscard]] int getIndent() const;
    void setIndent(int indent);
    void indent();
    void dedent();
private:
    int indentLevel = 0;
    std::string contents;
};
