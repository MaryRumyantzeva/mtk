#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <sstream>
#include <cctype>

using namespace std;

enum TokenType {
    KEYWORD, IDENTIFIER, OPERATOR, DELIMITER,
    CONSTANT_INT, CONSTANT_FLOAT, CONSTANT_STRING, END_OF_FILE
};

struct Token {
    TokenType type;
    string value;
    int line;
};

vector<Token> loadTokens(const string& filename) {
    vector<Token> tokens;
    ifstream file(filename);
    if (!file.is_open()) {
        cerr << "Cannot open " << filename << endl;
        return tokens;
    }

    string content, line;
    while (getline(file, line)) {
        content += line;
    }
    file.close();

    size_t pos = 0;
    int lineNum = 1;

    size_t startSeq = content.find("===");
    if (startSeq != string::npos) {
        pos = content.find('[', startSeq);
        if (pos == string::npos) pos = 0;
    }

    while (pos < content.length()) {
        size_t start = content.find('(', pos);
        if (start == string::npos) break;
        size_t end = content.find(')', start);
        if (end == string::npos) break;

        string tokenStr = content.substr(start + 1, end - start - 1);

        if (tokenStr.empty() || tokenStr == " " || tokenStr == "TOKENS" ||
            tokenStr == "SEQUENCE" || tokenStr.find("===") != string::npos ||
            tokenStr == "END_OF_FILE") {
            pos = end + 1;
            continue;
        }

        size_t comma = tokenStr.find(',');
        if (comma != string::npos) {
            string typeStr = tokenStr.substr(0, comma);
            string valStr = tokenStr.substr(comma + 1);

            while (!typeStr.empty() && isspace(typeStr.front())) typeStr.erase(0, 1);
            while (!typeStr.empty() && isspace(typeStr.back())) typeStr.pop_back();
            while (!valStr.empty() && isspace(valStr.front())) valStr.erase(0, 1);
            while (!valStr.empty() && isspace(valStr.back())) valStr.pop_back();

            if (typeStr.empty() || valStr.empty()) {
                pos = end + 1;
                continue;
            }

            Token t;
            t.value = valStr;
            t.line = lineNum;

            if (typeStr == "KEYWORD") t.type = KEYWORD;
            else if (typeStr == "IDENTIFIER") t.type = IDENTIFIER;
            else if (typeStr == "OPERATOR") t.type = OPERATOR;
            else if (typeStr == "DELIMITER") t.type = DELIMITER;
            else if (typeStr == "CONSTANT_INT") t.type = CONSTANT_INT;
            else if (typeStr == "CONSTANT_FLOAT") t.type = CONSTANT_FLOAT;
            else if (typeStr == "CONSTANT_STRING") t.type = CONSTANT_STRING;
            else t.type = END_OF_FILE;

            tokens.push_back(t);

            if (valStr == ";" || valStr == "{" || valStr == "}") lineNum++;
        }
        pos = end + 1;
    }

    cout << "Loaded " << tokens.size() << " tokens" << endl;
    return tokens;
}

class Parser {
private:
    vector<Token> tokens;
    int pos;
    vector<string> errors;
    int indent;
    int stepCount;

    Token current() {
        if (pos >= (int)tokens.size()) {
            Token e; e.type = END_OF_FILE; e.value = "EOF";
            return e;
        }
        return tokens[pos];
    }

    void advance() {
        if (pos < (int)tokens.size()) pos++;
    }

    bool check(const string& val) {
        return current().value == val;
    }

    bool checkType(TokenType type) {
        return current().type == type;
    }

    void print(const string& text) {
        for (int i = 0; i < indent; i++) cout << "  ";
        cout << text << endl;
    }

    void print(const string& name, const string& val) {
        for (int i = 0; i < indent; i++) cout << "  ";
        cout << name << ": " << val << endl;
    }

    bool checkStringQuotes(const string& str) {
        if (str.length() < 2) return false;
        return (str.front() == '"' && str.back() == '"');
    }

    // Функция для пропуска скобок без вывода ошибок
    void skipOptionalParen() {
        if (check(")")) {
            advance();
        }
    }

    string parseExpression() {
        string result;

        if (checkType(IDENTIFIER)) {
            result = current().value;
            advance();
            if (check("(")) {
                advance();
                result += "(";

                int argCount = 0;
                while (!check(")") && !check(";") && !checkType(END_OF_FILE)) {
                    if (argCount > 0 && check(",")) {
                        result += ", ";
                        advance();
                    }

                    if (checkType(IDENTIFIER)) {
                        result += current().value;
                        advance();
                        argCount++;
                    }
                    else if (checkType(CONSTANT_INT)) {
                        result += current().value;
                        advance();
                        argCount++;
                    }
                    else if (checkType(CONSTANT_FLOAT)) {
                        result += current().value;
                        advance();
                        argCount++;
                    }
                    else if (checkType(CONSTANT_STRING)) {
                        result += current().value;
                        advance();
                        argCount++;
                    }
                    else {
                        break;
                    }
                }

                result += ")";

                // Пропускаем закрывающую скобку, если она есть
                if (check(")")) {
                    advance();
                }
            }
        }
        else if (checkType(CONSTANT_INT)) {
            result = current().value;
            advance();
        }
        else if (checkType(CONSTANT_FLOAT)) {
            result = current().value;
            advance();
        }
        else if (checkType(CONSTANT_STRING)) {
            if (!checkStringQuotes(current().value)) {
                errors.push_back("Error: String literal missing quotes: '" + current().value + "'");
            }
            result = current().value;
            advance();
        }

        if (checkType(OPERATOR) && current().value != "++" && current().value != "--") {
            string op = current().value;
            advance();
            result = result + " " + op + " " + parseExpression();
        }

        return result;
    }

    void parseStatement() {
        stepCount++;
        if (stepCount > 5000) {
            errors.push_back("Error: Possible infinite loop, stopping");
            return;
        }

        if (checkType(END_OF_FILE)) return;

        if (check("if")) {
            print("IF");
            indent++;
            advance();

            if (check("(")) advance();
            // Не выводим ошибку, если нет '('

            string cond = parseExpression();
            print("CONDITION", cond);

            // Пропускаем ')' если есть, но не выводим ошибку
            skipOptionalParen();

            print("THEN");
            indent++;
            parseStatement();
            indent--;

            if (check("else")) {
                advance();
                print("ELSE");
                indent++;
                parseStatement();
                indent--;
            }
            indent--;
        }
        else if (check("for")) {
            print("FOR");
            indent++;
            advance();

            if (check("(")) advance();

            string init;
            if (check("int")) { init = "int "; advance(); }
            if (checkType(IDENTIFIER)) { init += current().value; advance(); }
            if (check("=")) { init += " = "; advance(); if (checkType(CONSTANT_INT)) { init += current().value; advance(); } }

            if (check(";")) advance();
            else errors.push_back("Error: Expected ';' after for initialization");
            print("INIT", init);

            string condition;
            if (checkType(IDENTIFIER)) {
                condition = current().value; advance();
                if (checkType(OPERATOR)) { condition += " " + current().value + " "; advance(); if (checkType(CONSTANT_INT)) { condition += current().value; advance(); } }
            }

            if (check(";")) advance();
            else errors.push_back("Error: Expected ';' after for condition");
            print("CONDITION", condition);

            string iteration;
            if (checkType(IDENTIFIER)) {
                iteration = current().value; advance();
                if (checkType(OPERATOR)) { iteration += current().value; advance(); }
            }

            skipOptionalParen(); // Пропускаем ')' если есть

            print("BODY");
            indent++;
            parseStatement();
            indent--;
            indent--;
        }
        else if (check("while")) {
            print("WHILE");
            indent++;
            advance();

            if (check("(")) advance();

            string cond = parseExpression();
            print("CONDITION", cond);

            skipOptionalParen(); // Пропускаем ')' если есть

            print("BODY");
            indent++;
            parseStatement();
            indent--;
            indent--;
        }
        else if (check("return")) {
            advance();
            string val;
            if (!check(";")) val = parseExpression();
            print("RETURN", val.empty() ? "void" : val);
            if (check(";")) advance();
            else errors.push_back("Error: Expected ';' after return");
        }
        else if (check("int") || check("bool")) {
            parseDeclaration();
        }
        else if (check("{")) {
            print("BLOCK");
            indent++;
            advance();
            int stmtCount = 0;
            while (!check("}") && !checkType(END_OF_FILE) && stmtCount < 1000) {
                parseStatement();
                stmtCount++;
                if (stepCount > 5000) break;
            }
            if (check("}")) advance();
            else errors.push_back("Error: Missing '}' to close block");
            indent--;
        }
        else if (check("cout")) {
            parseOutput();
        }
        else if (checkType(IDENTIFIER)) {
            parseAssignment();
        }
        else if (check(";")) {
            advance();
        }
        else {
            errors.push_back("Error: Unexpected token '" + current().value + "'");
            advance();
        }
    }

    void parseDeclaration() {
        string type = current().value;
        advance();

        if (!checkType(IDENTIFIER)) {
            errors.push_back("Error: Expected variable name after " + type);
            return;
        }

        string name = current().value;
        advance();

        string init;
        if (check("=")) {
            advance();
            if (checkType(CONSTANT_INT) || checkType(CONSTANT_FLOAT)) {
                init = current().value;
                advance();
            }
            else {
                errors.push_back("Error: Expected constant after '='");
            }
        }

        while (check(",")) {
            advance();
            if (checkType(IDENTIFIER)) advance();
            if (check("=")) { advance(); if (checkType(CONSTANT_INT)) advance(); }
        }

        if (check(";")) advance();
        else errors.push_back("Error: Expected ';' after declaration");

        if (init.empty()) print("DECLARATION", name + " : " + type);
        else print("DECLARATION", name + " : " + type + " = " + init);
    }

    void parseAssignment() {
        string left = current().value;
        advance();

        if (check("=")) {
            advance();
            string right = parseExpression();
            if (check(";")) advance();
            else errors.push_back("Error: Expected ';' after assignment (found '" + current().value + "')");
            print("ASSIGNMENT", left + " = " + right);
        }
        else if (check("++") || check("--")) {
            string op = current().value;
            advance();
            if (check(";")) advance();
            else errors.push_back("Error: Expected ';' after " + op);
            print("ASSIGNMENT", left + op);
        }
        else {
            errors.push_back("Error: Expected '=' or '++' after identifier");
        }
    }

    void parseOutput() {
        print("OUTPUT");
        indent++;
        advance();

        while (check("<<")) {
            advance();
            if (checkType(CONSTANT_STRING)) {
                if (!checkStringQuotes(current().value)) {
                    errors.push_back("Error: String missing quotes: " + current().value);
                }
                print("STRING", current().value);
                advance();
            }
            else if (checkType(IDENTIFIER)) {
                if (current().value == "endl") print("ENDL", "endl");
                else print("IDENTIFIER", current().value);
                advance();
            }
            else {
                errors.push_back("Error: Expected string or identifier after '<<'");
                break;
            }
        }

        if (check(";")) advance();
        else errors.push_back("Error: Expected ';' after output");
        indent--;
    }

    void parseFunction() {
        string retType = current().value;
        advance();

        if (!checkType(IDENTIFIER)) {
            errors.push_back("Error: Expected function name after " + retType);
            return;
        }

        string funcName = current().value;
        advance();

        print("FUNCTION", funcName + "() : " + retType);
        indent++;

        if (check("(")) {
            advance();
        }

        print("PARAMETERS");
        indent++;

        int paramCount = 0;
        while (!check(")") && !check("{") && !checkType(END_OF_FILE)) {
            if (check("int") || check("bool")) {
                string pType = current().value;
                advance();
                if (checkType(IDENTIFIER)) {
                    string pName = current().value;
                    advance();
                    print(pName, pType);
                    paramCount++;
                }
                else {
                    errors.push_back("Error: Expected parameter name");
                    break;
                }
            }
            else if (check(",")) {
                advance();
            }
            else {
                break;
            }
        }

        indent--;

        // Пропускаем ')' если есть
        skipOptionalParen();

        // Пропускаем лишнюю скобку, если есть
        if (check(")")) {
            advance();
        }

        // Проверяем тело функции
        if (check("{")) {
            print("BLOCK");
            indent++;
            advance();
            int stmtCount = 0;
            while (!check("}") && !checkType(END_OF_FILE) && stmtCount < 1000) {
                parseStatement();
                stmtCount++;
                if (stepCount > 5000) break;
            }
            if (check("}")) {
                advance();
            }
            else {
                errors.push_back("Error: Missing '}' for function body");
            }
            indent--;
        }
        else {
            errors.push_back("Error: Expected '{' for function body (found '" + current().value + "')");
        }

        indent--;
    }

    bool isFunction() {
        int save = pos;
        bool result = false;
        if (check("int") || check("bool")) {
            advance();
            if (checkType(IDENTIFIER)) {
                advance();
                if (check("(")) result = true;
            }
        }
        pos = save;
        return result;
    }

public:
    Parser(const vector<Token>& t) : tokens(t), pos(0), indent(0), stepCount(0) {}

    void parse() {
        cout << "\n========================================" << endl;
        cout << "   ABSTRACT SYNTAX TREE (AST)" << endl;
        cout << "========================================\n" << endl;

        print("PROGRAM");
        indent++;

        if (check("using")) {
            advance();
            if (check("namespace")) advance();
            if (check("std")) { advance(); print("USING", "std"); }
            if (check(";")) advance();
            else errors.push_back("Error: Expected ';' after using namespace std");
        }

        print("DECLARATIONS");
        indent++;
        int declCount = 0;
        while (!isFunction() && (check("int") || check("bool")) && declCount < 100) {
            parseDeclaration();
            declCount++;
        }
        indent--;

        print("FUNCTIONS");
        indent++;
        int funcCount = 0;
        while (!checkType(END_OF_FILE) && funcCount < 100) {
            if ((check("int") || check("bool")) && isFunction()) {
                parseFunction();
                funcCount++;
            }
            else {
                break;
            }
        }
        indent--;
        indent--;

        cout << endl;
        cout << "========================================" << endl;
        if (errors.empty()) {
            cout << "Syntax analysis completed successfully!" << endl;
            cout << "No errors detected." << endl;
        }
        else {
            cout << "SYNTAX ERRORS FOUND (" << errors.size() << "):" << endl;
            for (const auto& e : errors) {
                cout << "  " << e << endl;
            }
        }
        cout << "========================================" << endl;
    }
};

int main() {
    cout << "   SYNTAX ANALYZER (Parser) - Lab 3" << endl;

    vector<Token> tokens = loadTokens("tokens_output.txt");
    if (tokens.empty()) {
        cout << "Error: No tokens loaded!" << endl;
        return 1;
    }

    cout << "Processing " << tokens.size() << " tokens..." << endl;

    Parser parser(tokens);
    parser.parse();

    return 0;
}