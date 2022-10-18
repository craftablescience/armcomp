#pragma once

#include <string>

class FileWriter {
public:
    FileWriter() = default;
    FileWriter& operator<<(const std::string& line);
    [[nodiscard]] const std::string& getContents() const;
    void writeLine(const std::string& line, bool newline = true);
    void indent();
    void dedent();
private:
    int indentLevel = 0;
    std::string contents;
};
