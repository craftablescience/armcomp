#include "filewriter.hpp"

const std::string& FileWriter::getContents() const {
    return this->contents;
}

int FileWriter::getIndent() const {
    return this->indentLevel;
}

void FileWriter::setIndent(int indent) {
    this->indentLevel = indent;
}

void FileWriter::indent() {
    this->indentLevel++;
}

void FileWriter::dedent() {
    this->indentLevel--;
}
