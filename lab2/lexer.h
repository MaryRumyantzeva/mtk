#pragma once
#ifndef LEXER_H
#define LEXER_H

#include <string>
#include <vector>
#include <map>
#include <regex>

using namespace std;

enum class TokenType {
    KEYWORD,
    IDENTIFIER,
    OPERATOR,
    DELIMITER,
    CONSTANT_INT,
    CONSTANT_FLOAT,
    CONSTANT_STRING,
    CONSTANT_BOOL,
    END_OF_FILE,
    ERROR
};

struct Token {
    TokenType type;
    string value;
    int line;
    int column;

    Token(TokenType t, const string& v, int l, int c)
        : type(t), value(v), line(l), column(c) {}
};

class Lexer {
private:
    string source;
    int position;
    int line;
    int column;
    vector<Token> tokens;
    vector<string> errors;

    // Таблицы лексем
    map<string, TokenType> keywords;
    map<string, TokenType> operators;
    map<string, TokenType> delimiters;
    map<string, TokenType> booleanConstants;

    void initTables();
    char peek() const;
    char getChar();
    void skipWhitespace();
    void skipDirective();

    Token readIdentifier();
    Token readNumber();
    Token readString();
    Token readOperatorOrDelimiter();
    Token readUnknown();

    void addToken(TokenType type, const string& value);

public:
    Lexer(const string& sourceCode);
    void analyze();
    void printTokens() const;
    void printErrors() const;
    vector<Token> getTokens() const;
    bool hasErrors() const;
};

#endif