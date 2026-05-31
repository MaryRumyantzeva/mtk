#include "lexer.h"
#include <iostream>
#include <cctype>

using namespace std;

Lexer::Lexer(const string& sourceCode)
    : source(sourceCode), position(0), line(1), column(1) {
    initTables();
}

void Lexer::initTables() {
    // Ключевые слова
    keywords["int"] = TokenType::KEYWORD;
    keywords["bool"] = TokenType::KEYWORD;
    keywords["return"] = TokenType::KEYWORD;
    keywords["if"] = TokenType::KEYWORD;
    keywords["else"] = TokenType::KEYWORD;
    keywords["for"] = TokenType::KEYWORD;
    keywords["while"] = TokenType::KEYWORD;
    keywords["using"] = TokenType::KEYWORD;
    keywords["namespace"] = TokenType::KEYWORD;
    keywords["include"] = TokenType::KEYWORD;
    keywords["true"] = TokenType::CONSTANT_BOOL;
    keywords["false"] = TokenType::CONSTANT_BOOL;
    keywords["cout"] = TokenType::IDENTIFIER;
    keywords["cin"] = TokenType::IDENTIFIER;
    keywords["endl"] = TokenType::IDENTIFIER;
    keywords["std"] = TokenType::IDENTIFIER;
    keywords["string"] = TokenType::KEYWORD;
    keywords["iostream"] = TokenType::IDENTIFIER;

    // Операторы
    operators["="] = TokenType::OPERATOR;
    operators["+"] = TokenType::OPERATOR;
    operators["-"] = TokenType::OPERATOR;
    operators["*"] = TokenType::OPERATOR;
    operators["/"] = TokenType::OPERATOR;
    operators["%"] = TokenType::OPERATOR;
    operators["=="] = TokenType::OPERATOR;
    operators["!="] = TokenType::OPERATOR;
    operators["<"] = TokenType::OPERATOR;
    operators[">"] = TokenType::OPERATOR;
    operators["<="] = TokenType::OPERATOR;
    operators[">="] = TokenType::OPERATOR;
    operators["&&"] = TokenType::OPERATOR;
    operators["||"] = TokenType::OPERATOR;
    operators["!"] = TokenType::OPERATOR;
    operators["++"] = TokenType::OPERATOR;
    operators["--"] = TokenType::OPERATOR;
    operators["+="] = TokenType::OPERATOR;
    operators["-="] = TokenType::OPERATOR;
    operators["<<"] = TokenType::OPERATOR;
    operators[">>"] = TokenType::OPERATOR;

    // Разделители
    delimiters[";"] = TokenType::DELIMITER;
    delimiters["{"] = TokenType::DELIMITER;
    delimiters["}"] = TokenType::DELIMITER;
    delimiters["("] = TokenType::DELIMITER;
    delimiters[")"] = TokenType::DELIMITER;
    delimiters[","] = TokenType::DELIMITER;
    delimiters[":"] = TokenType::DELIMITER;
    delimiters["::"] = TokenType::DELIMITER;
    delimiters["#"] = TokenType::DELIMITER;
}

char Lexer::peek() const {
    if (position >= source.length()) return '\0';
    return source[position];
}

char Lexer::getChar() {
    if (position >= source.length()) return '\0';
    char c = source[position];
    position++;
    if (c == '\n') {
        line++;
        column = 1;
    }
    else {
        column++;
    }
    return c;
}

void Lexer::skipWhitespace() {
    while (isspace(peek())) {
        getChar();
    }
}

void Lexer::skipDirective() {
    while (peek() != '\n' && peek() != '\0') {
        getChar();
    }
}

Token Lexer::readIdentifier() {
    string value;
    int startLine = line;
    int startCol = column;

    while (isalnum(peek()) || peek() == '_') {
        value += getChar();
    }

    auto it = keywords.find(value);
    if (it != keywords.end()) {
        return Token(it->second, value, startLine, startCol);
    }

    if (value == "true" || value == "false") {
        return Token(TokenType::CONSTANT_BOOL, value, startLine, startCol);
    }

    return Token(TokenType::IDENTIFIER, value, startLine, startCol);
}

Token Lexer::readNumber() {
    string value;
    int startLine = line;
    int startCol = column;
    bool hasDot = false;

    while (isdigit(peek()) || peek() == '.') {
        if (peek() == '.') {
            if (hasDot) {
                errors.push_back("Error at " + to_string(line) + ":" + to_string(column) +
                    " - Invalid number: multiple dots");
                return Token(TokenType::ERROR, value + ".", startLine, startCol);
            }
            hasDot = true;
        }
        value += getChar();
    }

    if (isalpha(peek())) {
        errors.push_back("Error at " + to_string(line) + ":" + to_string(column) +
            " - Invalid number: letters in numeric constant");
        while (isalnum(peek())) {
            getChar();
        }
        return Token(TokenType::ERROR, value, startLine, startCol);
    }

    if (hasDot) {
        return Token(TokenType::CONSTANT_FLOAT, value, startLine, startCol);
    }
    return Token(TokenType::CONSTANT_INT, value, startLine, startCol);
}

Token Lexer::readString() {
    string value;
    int startLine = line;
    int startCol = column;

    getChar(); 

    while (peek() != '"' && peek() != '\0' && peek() != '\n') {
        value += getChar();
    }

    if (peek() == '"') {
        getChar(); 
        return Token(TokenType::CONSTANT_STRING, value, startLine, startCol);
    }
    else {
        errors.push_back("Error at " + to_string(line) + ":" + to_string(column) +
            " - Unterminated string literal");
        return Token(TokenType::ERROR, value, startLine, startCol);
    }
}

Token Lexer::readOperatorOrDelimiter() {
    string value;
    int startLine = line;
    int startCol = column;

    value += getChar();

    string twoChars = value + peek();
    if (operators.find(twoChars) != operators.end()) {
        value += getChar();
        return Token(TokenType::OPERATOR, value, startLine, startCol);
    }
    if (delimiters.find(twoChars) != delimiters.end()) {
        value += getChar();
        return Token(TokenType::DELIMITER, value, startLine, startCol);
    }

    if (operators.find(value) != operators.end()) {
        return Token(TokenType::OPERATOR, value, startLine, startCol);
    }
    if (delimiters.find(value) != delimiters.end()) {
        return Token(TokenType::DELIMITER, value, startLine, startCol);
    }

    errors.push_back("Error at " + to_string(line) + ":" + to_string(column) +
        " - Unknown character: '" + value + "'");
    return Token(TokenType::ERROR, value, startLine, startCol);
}

Token Lexer::readUnknown() {
    string value;
    int startLine = line;
    int startCol = column;
    value += getChar();

    errors.push_back("Error at " + to_string(line) + ":" + to_string(column) +
        " - Unknown token: '" + value + "'");
    return Token(TokenType::ERROR, value, startLine, startCol);
}

void Lexer::addToken(TokenType type, const string& value) {
    tokens.push_back(Token(type, value, line, column));
}

void Lexer::analyze() {
    while (position < source.length()) {
        skipWhitespace();

        if (position >= source.length()) break;

        char c = peek();

        if (c == '#') {
            getChar();
            skipDirective();
            continue;
        }

        if (isalpha(c) || c == '_') {
            tokens.push_back(readIdentifier());
        }
 
        else if (isdigit(c)) {
            tokens.push_back(readNumber());
        }

        else if (c == '"') {
            tokens.push_back(readString());
        }
 
        else if (ispunct(c)) {
            tokens.push_back(readOperatorOrDelimiter());
        }

        else {
            tokens.push_back(readUnknown());
        }
    }

    tokens.push_back(Token(TokenType::END_OF_FILE, "", line, column));
}

void Lexer::printTokens() const {
    cout << "\n" << string(60, '=') << endl;
    cout << "LEXICAL ANALYSIS RESULTS" << endl;
    cout << string(60, '=') << endl;

    printf("%-20s | %-15s | %-10s\n", "Lexeme", "Type", "Position");
    cout << string(60, '-') << endl;

    for (const auto& token : tokens) {
        if (token.type == TokenType::END_OF_FILE) continue;

        string typeStr;
        switch (token.type) {
        case TokenType::KEYWORD: typeStr = "KEYWORD"; break;
        case TokenType::IDENTIFIER: typeStr = "IDENTIFIER"; break;
        case TokenType::OPERATOR: typeStr = "OPERATOR"; break;
        case TokenType::DELIMITER: typeStr = "DELIMITER"; break;
        case TokenType::CONSTANT_INT: typeStr = "CONSTANT_INT"; break;
        case TokenType::CONSTANT_FLOAT: typeStr = "CONSTANT_FLOAT"; break;
        case TokenType::CONSTANT_STRING: typeStr = "CONSTANT_STRING"; break;
        case TokenType::CONSTANT_BOOL: typeStr = "CONSTANT_BOOL"; break;
        case TokenType::ERROR: typeStr = "ERROR"; break;
        default: typeStr = "UNKNOWN";
        }

        string displayValue = token.value;
        if (displayValue.empty()) displayValue = "(empty)";
        if (displayValue == "\n") displayValue = "\\n";

        printf("%-20s | %-15s | %d:%d\n",
            displayValue.c_str(), typeStr.c_str(), token.line, token.column);
    }
}

void Lexer::printErrors() const {
    if (!errors.empty()) {
        cout << "\n" << string(60, '=') << endl;
        cout << "ERRORS FOUND" << endl;
        cout << string(60, '=') << endl;
        for (const auto& error : errors) {
            cout << error << endl;
        }
    }
    else {
        cout << "\nNo errors detected." << endl;
    }
}

vector<Token> Lexer::getTokens() const {
    return tokens;
}

bool Lexer::hasErrors() const {
    return !errors.empty();
}