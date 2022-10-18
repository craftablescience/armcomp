#include "filewriter.hpp"

FileWriter& FileWriter::operator<<(const std::string& line) {
    this->writeLine(line);
    return *this;
}

const std::string& FileWriter::getContents() const {
    return this->contents;
}

void FileWriter::writeLine(const std::string& line, bool newline) {
    for (int i = 0; i < this->indentLevel; i++) {
        this->contents += "    ";
    }
    this->contents += line + (newline ? "\n" : "");
}

void FileWriter::indent() {
    this->indentLevel++;
}

void FileWriter::dedent() {
    this->indentLevel--;
}
