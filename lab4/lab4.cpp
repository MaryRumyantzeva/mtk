#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <sstream>
#include <cctype>
#include <regex>

using namespace std;


// ЛАБОРАТОРНАЯ РАБОТА №1: ПРЕПРОЦЕССОР

class Preprocessor {
private:
    string inputFileName;
    string outputFileName;
    vector<string> errors;

    string removeBOM(const string& text) {
        if (text.length() >= 3) {
            unsigned char c1 = static_cast<unsigned char>(text[0]);
            unsigned char c2 = static_cast<unsigned char>(text[1]);
            unsigned char c3 = static_cast<unsigned char>(text[2]);
            if (c1 == 0xEF && c2 == 0xBB && c3 == 0xBF) {
                return text.substr(3);
            }
        }
        return text;
    }

    bool checkMultiLineComments(const string& code) {
        regex multiLineCommentStart("/\\*");
        regex multiLineCommentEnd("\\*/");

        auto startBegin = sregex_iterator(code.begin(), code.end(), multiLineCommentStart);
        auto startEnd = sregex_iterator();
        auto endBegin = sregex_iterator(code.begin(), code.end(), multiLineCommentEnd);
        auto endEnd = sregex_iterator();

        int openCount = distance(startBegin, startEnd);
        int closeCount = distance(endBegin, endEnd);

        if (openCount != closeCount) {
            errors.push_back("Preprocessor error: Unclosed multi-line comment /*");
            return false;
        }
        return true;
    }

    string removeMultiLineComments(const string& code) {
        regex multiLineComment("/\\*[\\s\\S]*?\\*/");
        return regex_replace(code, multiLineComment, "");
    }

    string removeSingleLineComments(const string& code) {
        regex singleLineComment("//.*$");
        istringstream stream(code);
        string line;
        string result;
        while (getline(stream, line)) {
            string cleanedLine = regex_replace(line, singleLineComment, "");
            result += cleanedLine + "\n";
        }
        return result;
    }

    vector<string> normalizeWhitespace(const string& code) {
        vector<string> result;
        istringstream stream(code);
        string line;
        while (getline(stream, line)) {
            regex whitespace("\\s+");
            string normalized = regex_replace(line, whitespace, " ");
            normalized = regex_replace(normalized, regex("^\\s+|\\s+$"), "");
            if (!normalized.empty()) {
                result.push_back(normalized);
            }
        }
        return result;
    }

public:
    Preprocessor(const string& input, const string& output)
        : inputFileName(input), outputFileName(output) {}

    bool process() {
        ifstream inputFile(inputFileName);
        if (!inputFile.is_open()) {
            errors.push_back("Error: cannot open input file " + inputFileName);
            return false;
        }

        stringstream buffer;
        buffer << inputFile.rdbuf();
        string code = buffer.str();
        inputFile.close();

        code = removeBOM(code);

        // Проверка на незакрытые комментарии
        if (!checkMultiLineComments(code)) {
            return false;
        }

        code = removeMultiLineComments(code);
        code = removeSingleLineComments(code);
        vector<string> cleanedLines = normalizeWhitespace(code);

        ofstream outputFile(outputFileName);
        if (!outputFile.is_open()) {
            errors.push_back("Error: cannot create output file " + outputFileName);
            return false;
        }

        for (const auto& line : cleanedLines) {
            outputFile << line << endl;
        }
        outputFile.close();

        cout << "  [Preprocessor] File processed: " << inputFileName << " -> " << outputFileName << endl;
        return true;
    }

    bool hasErrors() const {
        return !errors.empty();
    }

    void printErrors() const {
        for (const auto& e : errors) {
            cout << "  " << e << endl;
        }
    }
};


// ЛАБОРАТОРНАЯ РАБОТА №2: ЛЕКСИЧЕСКИЙ АНАЛИЗАТОР

enum TokenType {
    KEYWORD, IDENTIFIER, OPERATOR, DELIMITER,
    CONSTANT_INT, CONSTANT_FLOAT, CONSTANT_STRING, CONSTANT_BOOL, ERROR, END_OF_FILE
};

struct Token {
    TokenType type;
    string value;
    int line;
    int column;

    Token() : type(END_OF_FILE), value(""), line(0), column(0) {}
    Token(TokenType t, const string& v, int l, int c) : type(t), value(v), line(l), column(c) {}
};

class Lexer {
private:
    string source;
    size_t position;
    int line;
    int column;
    vector<Token> tokens;
    vector<string> errors;
    map<string, TokenType> keywords;
    map<string, TokenType> operators;
    map<string, TokenType> delimiters;

    void initTables() {
        keywords["int"] = KEYWORD;
        keywords["string"] = KEYWORD;
        keywords["bool"] = KEYWORD;
        keywords["return"] = KEYWORD;
        keywords["if"] = KEYWORD;
        keywords["else"] = KEYWORD;
        keywords["for"] = KEYWORD;
        keywords["while"] = KEYWORD;
        keywords["using"] = KEYWORD;
        keywords["namespace"] = KEYWORD;
        keywords["true"] = CONSTANT_BOOL;
        keywords["false"] = CONSTANT_BOOL;
        keywords["cout"] = IDENTIFIER;
        keywords["endl"] = IDENTIFIER;
        keywords["std"] = IDENTIFIER;

        operators["="] = OPERATOR;
        operators["+"] = OPERATOR;
        operators["-"] = OPERATOR;
        operators["*"] = OPERATOR;
        operators["/"] = OPERATOR;
        operators["%"] = OPERATOR;
        operators["=="] = OPERATOR;
        operators["!="] = OPERATOR;
        operators["<"] = OPERATOR;
        operators[">"] = OPERATOR;
        operators["<="] = OPERATOR;
        operators[">="] = OPERATOR;
        operators["++"] = OPERATOR;
        operators["--"] = OPERATOR;
        operators["<<"] = OPERATOR;
        operators[">>"] = OPERATOR;

        delimiters[";"] = DELIMITER;
        delimiters["{"] = DELIMITER;
        delimiters["}"] = DELIMITER;
        delimiters["("] = DELIMITER;
        delimiters[")"] = DELIMITER;
        delimiters[","] = DELIMITER;
        delimiters[":"] = DELIMITER;
        delimiters["::"] = DELIMITER;
    }

    char peek() const {
        if (position >= source.length()) return '\0';
        return source[position];
    }

    char getChar() {
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

    void skipWhitespace() {
        while (isspace(peek())) {
            getChar();
        }
    }

    Token readIdentifier() {
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
        return Token(IDENTIFIER, value, startLine, startCol);
    }

    Token readNumber() {
        string value;
        int startLine = line;
        int startCol = column;
        bool hasDot = false;

        while (isdigit(peek()) || peek() == '.') {
            if (peek() == '.') {
                if (hasDot) {
                    errors.push_back("Error at " + to_string(line) + ":" + to_string(column) +
                        " - Invalid number: multiple dots");
                    return Token(ERROR, value + ".", startLine, startCol);
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
            return Token(ERROR, value, startLine, startCol);
        }

        if (hasDot) {
            return Token(CONSTANT_FLOAT, value, startLine, startCol);
        }
        return Token(CONSTANT_INT, value, startLine, startCol);
    }

    Token readString() {
        string value;
        int startLine = line;
        int startCol = column;
        getChar();

        while (peek() != '"' && peek() != '\0' && peek() != '\n') {
            value += getChar();
        }

        if (peek() == '"') {
            getChar();
            return Token(CONSTANT_STRING, value, startLine, startCol);
        }
        else {
            errors.push_back("Error at " + to_string(line) + ":" + to_string(column) +
                " - Unterminated string literal");
            return Token(ERROR, value, startLine, startCol);
        }
    }

    Token readOperatorOrDelimiter() {
        string value;
        int startLine = line;
        int startCol = column;
        value += getChar();

        string twoChars = value + peek();
        if (operators.find(twoChars) != operators.end()) {
            value += getChar();
            return Token(OPERATOR, value, startLine, startCol);
        }
        if (delimiters.find(twoChars) != delimiters.end()) {
            value += getChar();
            return Token(DELIMITER, value, startLine, startCol);
        }

        if (operators.find(value) != operators.end()) {
            return Token(OPERATOR, value, startLine, startCol);
        }
        if (delimiters.find(value) != delimiters.end()) {
            return Token(DELIMITER, value, startLine, startCol);
        }

        errors.push_back("Error at " + to_string(line) + ":" + to_string(column) +
            " - Unknown character: '" + value + "'");
        return Token(ERROR, value, startLine, startCol);
    }

public:
    Lexer(const string& sourceCode) : source(sourceCode), position(0), line(1), column(1) {
        initTables();
    }

    void analyze() {
        while (position < source.length()) {
            skipWhitespace();
            if (position >= source.length()) break;

            char c = peek();

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
                string val;
                val += getChar();
                tokens.push_back(Token(ERROR, val, line, column));
                errors.push_back("Error at " + to_string(line) + ":" + to_string(column) +
                    " - Unknown character: '" + val + "'");
            }
        }
        tokens.push_back(Token(END_OF_FILE, "", line, column));
    }

    vector<Token> getTokens() const {
        return tokens;
    }

    bool hasErrors() const {
        return !errors.empty();
    }

    void printErrors() const {
        for (const auto& e : errors) {
            cout << "  " << e << endl;
        }
    }
};


// ЛАБОРАТОРНАЯ РАБОТА №3-4: СИНТАКСИЧЕСКИЙ + СЕМАНТИЧЕСКИЙ АНАЛИЗ

struct Symbol {
    string name;
    string type;
    bool declared;
    bool initialized;
    string scope;
    int line;
    int paramCount;
    vector<string> paramTypes;
};

struct Triad {
    string op;
    string operand1;
    string operand2;
};

class SemanticAnalyzer {
private:
    vector<Token> tokens;
    int pos;
    vector<string> syntaxErrors;
    vector<string> semanticErrors;
    int indent;
    int stepCount;

    map<string, Symbol> symbolTable;
    string currentScope;
    string currentFunction;
    string currentFunctionReturnType;

    vector<Triad> triads;
    int triadCounter;

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

    void skipOptionalParen() {
        if (check(")")) {
            advance();
        }
    }

    bool addSymbol(const string& name, const string& type, int line, bool initialized = false) {
        string key = currentScope + ":" + name;
        if (symbolTable.find(key) != symbolTable.end()) {
            semanticErrors.push_back("Semantic error: Variable '" + name + "' already declared in scope '" + currentScope + "'");
            return false;
        }

        Symbol sym;
        sym.name = name;
        sym.type = type;
        sym.declared = true;
        sym.initialized = initialized;
        sym.scope = currentScope;
        sym.line = line;
        symbolTable[key] = sym;
        return true;
    }

    Symbol* findSymbol(const string& name) {
        string key = currentScope + ":" + name;
        if (symbolTable.find(key) != symbolTable.end()) {
            return &symbolTable[key];
        }

        key = "global:" + name;
        if (symbolTable.find(key) != symbolTable.end()) {
            return &symbolTable[key];
        }

        return nullptr;
    }

    int addTriad(const string& op, const string& arg1, const string& arg2) {
        Triad t;
        t.op = op;
        t.operand1 = arg1;
        t.operand2 = arg2;
        triads.push_back(t);
        triadCounter++;
        return triads.size();  
    }

    string getExpressionType() {
        if (checkType(IDENTIFIER)) {
            Symbol* sym = findSymbol(current().value);
            if (sym) {
                return sym->type;
            }
            return "unknown";
        }
        else if (checkType(CONSTANT_INT)) {
            return "int";
        }
        else if (checkType(CONSTANT_FLOAT)) {
            return "float";
        }
        else if (checkType(CONSTANT_STRING)) {
            return "string";  
        }
        else if (checkType(CONSTANT_BOOL)) {
            return "bool";
        }
        return "unknown";
    }

    string parseExpression() {
        string result;

        if (checkType(IDENTIFIER)) {
            string varName = current().value;

            if (varName == "endl" || varName == "cout") {
                result = varName;
                advance();
                return result;
            }

            Symbol* sym = findSymbol(varName);
            if (!sym) {
                semanticErrors.push_back("Semantic error: Variable '" + varName + "' not declared");
            }

            result = varName;
            advance();

            if (check("(")) {
                advance();
                result += "(";

                int argCount = 0;
                vector<string> argTypes;

                while (!check(")") && !check(";") && !checkType(END_OF_FILE)) {
                    if (argCount > 0 && check(",")) {
                        result += ", ";
                        advance();
                    }

                    if (checkType(IDENTIFIER)) {
                        string argName = current().value;
                        Symbol* argSym = findSymbol(argName);
                        if (argSym) {
                            argTypes.push_back(argSym->type);
                        }
                        result += current().value;
                        advance();
                        argCount++;
                    }
                    else if (checkType(CONSTANT_INT)) {
                        argTypes.push_back("int");
                        result += current().value;
                        advance();
                        argCount++;
                    }
                    else if (checkType(CONSTANT_FLOAT)) {
                        argTypes.push_back("float");
                        result += current().value;
                        advance();
                        argCount++;
                    }
                    else if (checkType(CONSTANT_STRING)) {
                        argTypes.push_back("string");
                        result += "\"" + current().value + "\"";
                        advance();
                        argCount++;
                    }
                    else {
                        break;
                    }
                }

                result += ")";

                if (check(")")) {
                    advance();
                }
                else {
                    syntaxErrors.push_back("Syntax error: Missing ')' in function call");
                }

                if (sym && sym->type.find("function") != string::npos) {
                    if (argCount != sym->paramCount) {
                        semanticErrors.push_back("Semantic error: Function '" + varName + "' expects " +
                            to_string(sym->paramCount) + " arguments, got " + to_string(argCount));
                    }
                }

                return result;
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
            result = "\"" + current().value + "\"";
            advance();
        }
        else if (checkType(CONSTANT_BOOL)) {
            result = current().value;
            advance();
        }

        if (checkType(OPERATOR) && current().value != "++" && current().value != "--") {
            string op = current().value;
            advance();
            string right = parseExpression();

            int triadNum = addTriad(op, result, right);
            result = "^" + to_string(triadNum);
        }

        return result;
    }

    void parseStatement() {
        stepCount++;
        if (stepCount > 5000) {
            syntaxErrors.push_back("Error: Possible infinite loop, stopping");
            return;
        }

        if (checkType(END_OF_FILE)) return;

        if (check("if")) {
            print("IF");
            indent++;
            advance();

            if (!check("(")) {
                syntaxErrors.push_back("Syntax error: Expected '(' after if");
            }
            if (check("(")) advance();

            string cond = parseExpression();
            print("CONDITION", cond);

            if (!check(")")) {
                syntaxErrors.push_back("Syntax error: Expected ')' after condition");
            }
            skipOptionalParen();

            static int labelCounter = 0;
            int labelElse = labelCounter++;
            int labelEnd = labelCounter++;

            // Условный переход на else
            addTriad("if_false", cond, "L" + to_string(labelElse));

            print("THEN");
            indent++;
            parseStatement();
            indent--;

            // Переход на конец if
            addTriad("goto", "L" + to_string(labelEnd), "");

            // Метка для else
            addTriad("label", "L" + to_string(labelElse), "");

            if (check("else")) {
                advance();
                print("ELSE");
                indent++;
                parseStatement();
                indent--;
            }

            // Метка конца if
            addTriad("label", "L" + to_string(labelEnd), "");

            indent--;
        }
        else if (check("for")) {
            print("FOR");
            indent++;
            advance();

            if (!check("(")) {
                syntaxErrors.push_back("Syntax error: Expected '(' after for");
            }
            if (check("(")) advance();

            string init;
            string loopVarName;
            string loopVarType = "int";

            if (check("int")) {
                loopVarType = "int";
                init = "int ";
                advance();
            }
            if (checkType(IDENTIFIER)) {
                loopVarName = current().value;
                init += current().value;
                advance();
                addSymbol(loopVarName, loopVarType, current().line, true);
            }
            if (check("=")) {
                init += " = ";
                advance();
                if (checkType(CONSTANT_INT)) {
                    init += current().value;
                    advance();
                }
            }

            if (!check(";")) {
                syntaxErrors.push_back("Syntax error: Expected ';' after for initialization");
            }
            if (check(";")) advance();
            print("INIT", init);

            string condition;
            if (checkType(IDENTIFIER)) {
                condition = current().value;
                advance();
                if (checkType(OPERATOR)) {
                    condition += " " + current().value + " ";
                    advance();
                    if (checkType(CONSTANT_INT)) {
                        condition += current().value;
                        advance();
                    }
                }
            }

            if (!check(";")) {
                syntaxErrors.push_back("Syntax error: Expected ';' after for condition");
            }
            if (check(";")) advance();
            print("CONDITION", condition);

            string iteration;
            if (checkType(IDENTIFIER)) {
                iteration = current().value;
                advance();
                if (checkType(OPERATOR)) {
                    iteration += current().value;
                    advance();
                }
            }

            if (!check(")")) {
                syntaxErrors.push_back("Syntax error: Expected ')' after for iteration");
            }
            skipOptionalParen();

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

            if (!check("(")) {
                syntaxErrors.push_back("Syntax error: Expected '(' after while");
            }
            if (check("(")) advance();

            string cond = parseExpression();
            print("CONDITION", cond);

            if (!check(")")) {
                syntaxErrors.push_back("Syntax error: Expected ')' after while condition");
            }
            skipOptionalParen();

            print("BODY");
            indent++;
            parseStatement();
            indent--;
            indent--;
        }
        else if (check("return")) {
            advance();
            string val;
            if (!check(";")) {
                val = parseExpression();
            }
            print("RETURN", val.empty() ? "void" : val);
            if (!check(";")) {
                syntaxErrors.push_back("Syntax error: Expected ';' after return");
            }
            if (check(";")) advance();
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
            if (!check("}")) {
                syntaxErrors.push_back("Syntax error: Missing '}' to close block");
            }
            if (check("}")) advance();
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
        else if (checkType(END_OF_FILE)) {
            // Конец файла - ничего не делаем
        }
        else {
            syntaxErrors.push_back("Syntax error: Unexpected token '" + current().value + "'");
            advance();
        }
    }

    void parseDeclaration() {
        string type = current().value;
        advance();

        if (!checkType(IDENTIFIER)) {
            syntaxErrors.push_back("Syntax error: Expected variable name after " + type);
            return;
        }

        string name = current().value;
        advance();

        bool initialized = false;
        string init;
        string initType;

        if (check("=")) {
            advance();
            initialized = true;

            if (checkType(CONSTANT_INT)) {
                init = current().value;
                initType = "int";
                advance();
            }
            else if (checkType(CONSTANT_FLOAT)) {
                init = current().value;
                initType = "float";
                advance();
            }
            else if (checkType(CONSTANT_STRING)) {
                init = "\"" + current().value + "\"";
                initType = "string";
                advance();
            }
            else if (checkType(CONSTANT_BOOL)) {
                init = current().value;
                initType = "bool";
                advance();
            }
            else if (checkType(IDENTIFIER)) {
                // Присваивание другой переменной
                init = current().value;
                Symbol* sym = findSymbol(init);
                if (sym) {
                    initType = sym->type;
                }
                advance();
            }

            // ★★★ ПРОВЕРКА ТИПОВ ПРИ ОБЪЯВЛЕНИИ ★★★
            if (type != initType && !initType.empty()) {
                semanticErrors.push_back("Semantic error at line " + to_string(current().line) +
                    ": Cannot initialize variable '" + name + "' of type '" + type +
                    "' with value of type '" + initType + "'");
            }
        }

        // Проверка на повторное объявление
        string key = currentScope + ":" + name;
        if (symbolTable.find(key) != symbolTable.end()) {
            semanticErrors.push_back("Semantic error at line " + to_string(current().line) +
                ": Variable '" + name + "' already declared in scope '" + currentScope + "'");
        }

        // Добавляем в таблицу символов
        addSymbol(name, type, current().line, initialized);

        // Обработка нескольких переменных через запятую
        while (check(",")) {
            advance();
            if (checkType(IDENTIFIER)) {
                string varName = current().value;
                advance();
                bool varInit = false;
                string varInitType;
                string varInitValue;

                if (check("=")) {
                    advance();
                    varInit = true;
                    if (checkType(CONSTANT_INT)) {
                        varInitValue = current().value;
                        varInitType = "int";
                        advance();
                    }
                    else if (checkType(CONSTANT_STRING)) {
                        varInitValue = "\"" + current().value + "\"";
                        varInitType = "string";
                        advance();
                    }
                    else if (checkType(CONSTANT_FLOAT)) {
                        varInitValue = current().value;
                        varInitType = "float";
                        advance();
                    }

                    // ★★★ ПРОВЕРКА ТИПОВ ДЛЯ КАЖДОЙ ПЕРЕМЕННОЙ ★★★
                    if (type != varInitType && !varInitType.empty()) {
                        semanticErrors.push_back("Semantic error at line " + to_string(current().line) +
                            ": Cannot initialize variable '" + varName + "' of type '" + type +
                            "' with value of type '" + varInitType + "'");
                    }
                }

                // Проверка на повторное объявление
                string varKey = currentScope + ":" + varName;
                if (symbolTable.find(varKey) != symbolTable.end()) {
                    semanticErrors.push_back("Semantic error: Variable '" + varName +
                        "' already declared in scope '" + currentScope + "'");
                }

                addSymbol(varName, type, current().line, varInit);
            }
        }

        if (!check(";")) {
            syntaxErrors.push_back("Syntax error: Expected ';' after declaration");
        }
        if (check(";")) advance();

        if (init.empty()) print("DECLARATION", name + " : " + type);
        else print("DECLARATION", name + " : " + type + " = " + init);
    }

    void parseAssignment() {
        string left = current().value;

        Symbol* sym = findSymbol(left);
        if (!sym) {
            semanticErrors.push_back("Semantic error: Variable '" + left + "' not declared before assignment");
        }

        advance();

        if (check("=")) {
            advance();
            string right = parseExpression();

            // ★★★ ОПРЕДЕЛЯЕМ ТИП ПРАВОЙ ЧАСТИ ★★★
            string rightType = getExpressionType();

            // ★★★ СЕМАНТИЧЕСКАЯ ПРОВЕРКА ТИПОВ ПРИ ПРИСВАИВАНИИ ★★★
            if (sym && rightType != "unknown") {
                if (sym->type != rightType) {
                    semanticErrors.push_back("Semantic error: Type mismatch in assignment to '" + left +
                        "'. Expected '" + sym->type + "', got '" + rightType + "'");
                }
                sym->initialized = true;
            }

            // Генерируем триаду для присваивания
            addTriad(":=", left, right);

            if (!check(";")) {
                syntaxErrors.push_back("Syntax error: Expected ';' after assignment");
            }
            if (check(";")) advance();
            print("ASSIGNMENT", left + " = " + right);
        }
        else if (check("++") || check("--")) {
            string op = current().value;
            advance();
            if (!check(";")) {
                syntaxErrors.push_back("Syntax error: Expected ';' after " + op);
            }
            if (check(";")) advance();
            print("ASSIGNMENT", left + op);
        }
        else {
            syntaxErrors.push_back("Syntax error: Expected '=' or '++' after identifier");
        }
    }

    void parseOutput() {
        print("OUTPUT");
        indent++;
        advance();

        while (check("<<")) {
            advance();

            if (checkType(CONSTANT_STRING)) {
                int triadNum = addTriad("<<", "cout", "\"" + current().value + "\"");
                print("STRING", current().value);
                advance();
            }
            else if (checkType(IDENTIFIER)) {
                string varName = current().value;

                if (varName == "endl") {
                    int triadNum = addTriad("<<", "cout", "endl");
                    print("ENDL", "endl");
                    advance();
                    continue;
                }

                Symbol* sym = findSymbol(varName);
                if (!sym) {
                    semanticErrors.push_back("Semantic error: Variable '" + varName + "' not declared");
                }

                int triadNum = addTriad("<<", "cout", varName);
                print("IDENTIFIER", varName);
                advance();
            }
            else {
                syntaxErrors.push_back("Syntax error: Expected string or identifier after '<<'");
                break;
            }
        }

        if (!check(";")) {
            syntaxErrors.push_back("Syntax error: Expected ';' after output");
        }
        if (check(";")) advance();
        indent--;
    }

    void parseFunction() {
        string retType = current().value;
        advance();

        if (!checkType(IDENTIFIER)) {
            syntaxErrors.push_back("Syntax error: Expected function name after " + retType);
            return;
        }

        string funcName = current().value;
        advance();

        currentFunction = funcName;
        currentFunctionReturnType = retType;

        Symbol funcSym;
        funcSym.name = funcName;
        funcSym.type = retType + " function";
        funcSym.declared = true;
        funcSym.initialized = true;
        funcSym.scope = "global";
        funcSym.line = current().line;
        symbolTable["global:" + funcName] = funcSym;

        print("FUNCTION", funcName + "() : " + retType);
        indent++;

        currentScope = funcName;

        if (!check("(")) {
            syntaxErrors.push_back("Syntax error: Expected '(' after function name");
        }
        if (check("(")) {
            advance();
        }

        print("PARAMETERS");
        indent++;

        int paramCount = 0;
        vector<string> paramTypes;

        while (!check(")") && !check("{") && !checkType(END_OF_FILE)) {
            if (check("int") || check("bool")) {
                string pType = current().value;
                advance();
                if (checkType(IDENTIFIER)) {
                    string pName = current().value;
                    advance();
                    print(pName, pType);
                    paramCount++;
                    paramTypes.push_back(pType);
                    addSymbol(pName, pType, current().line, true);
                }
            }
            else if (check(",")) {
                advance();
            }
            else {
                break;
            }
        }

        symbolTable["global:" + funcName].paramCount = paramCount;
        symbolTable["global:" + funcName].paramTypes = paramTypes;

        indent--;

        if (!check(")")) {
            syntaxErrors.push_back("Syntax error: Expected ')' after function parameters");
        }
        skipOptionalParen();

        if (check(")")) {
            advance();
        }

        if (!check("{")) {
            syntaxErrors.push_back("Syntax error: Expected '{' for function body");
        }
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
            if (!check("}")) {
                syntaxErrors.push_back("Syntax error: Missing '}' for function body");
            }
            if (check("}")) {
                advance();
            }
            indent--;
        }

        indent--;

        currentScope = "global";
        currentFunction = "";
        currentFunctionReturnType = "";
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

    void printSymbolTable() {
        cout << "\n========================================" << endl;
        cout << "   SYMBOL TABLE" << endl;
        cout << "========================================\n" << endl;

        cout << "+--------------+--------------+------------+--------------+----------+" << endl;
        cout << "| Name         | Type         | Scope      | Initialized  | Line     |" << endl;
        cout << "+--------------+--------------+------------+--------------+----------+" << endl;

        for (const auto& entry : symbolTable) {
            const Symbol& s = entry.second;
            printf("| %-12s | %-12s | %-10s | %-12s | %-8d |\n",
                s.name.c_str(), s.type.c_str(), s.scope.c_str(),
                s.initialized ? "yes" : "no", s.line);
        }

        cout << "+--------------+--------------+------------+--------------+----------+" << endl;
    }

    void printTriads() {
        cout << "\n========================================" << endl;
        cout << "   INTERMEDIATE CODE (TRIADS)" << endl;
        cout << "========================================\n" << endl;

        cout << "+------+-------------+------------------+------------------+" << endl;
        cout << "| N   | Operation   | Operand 1        | Operand 2        |" << endl;
        cout << "+------+-------------+------------------+------------------+" << endl;

        for (size_t i = 0; i < triads.size(); i++) {
            printf("| %-4d | %-11s | %-16s | %-16s |\n",
                (int)i + 1, triads[i].op.c_str(),
                triads[i].operand1.c_str(), triads[i].operand2.c_str());
        }

        cout << "+------+-------------+------------------+------------------+" << endl;
    }

public:
    SemanticAnalyzer(const vector<Token>& t) : tokens(t), pos(0), indent(0), stepCount(0),
        triadCounter(0), currentScope("global") {}

    void analyze() {
        cout << "\n========================================" << endl;
        cout << "   SYNTAX ANALYSIS (AST)" << endl;
        cout << "========================================\n" << endl;

        print("PROGRAM");
        indent++;

        if (check("using")) {
            advance();
            if (check("namespace")) advance();
            if (check("std")) { advance(); print("USING", "std"); }
            if (!check(";")) {
                syntaxErrors.push_back("Syntax error: Expected ';' after using namespace std");
            }
            if (check(";")) advance();
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

        printSymbolTable();
        printTriads();


        bool hasSyntaxErrors = !syntaxErrors.empty();
        bool hasSemanticErrors = !semanticErrors.empty();

        if (!hasSyntaxErrors && !hasSemanticErrors) {
            cout << "Analysis completed successfully!" << endl;
            cout << "  No syntax or semantic errors detected." << endl;
        }
        else {
            if (hasSyntaxErrors) {
                cout << "SYNTAX ERRORS FOUND (" << syntaxErrors.size() << "):" << endl;
                for (const auto& e : syntaxErrors) {
                    cout << "  " << e << endl;
                }
            }
            if (hasSemanticErrors) {
                cout << "SEMANTIC ERRORS FOUND (" << semanticErrors.size() << "):" << endl;
                for (const auto& e : semanticErrors) {
                    cout << "  " << e << endl;
                }
            }
        }
        cout << "========================================" << endl;
    }

    bool hasErrors() const {
        return !syntaxErrors.empty() || !semanticErrors.empty();
    }
};


int main() {


    bool hasError = false;

    Preprocessor preprocessor("test.cpp", "test_cleaned.cpp");
    if (!preprocessor.process()) {
        cout << "Preprocessor failed with errors:" << endl;
        preprocessor.printErrors();
        cout << "Compilation terminated." << endl;
        return 1;
    }
    if (preprocessor.hasErrors()) {
        hasError = true;
        preprocessor.printErrors();
    }
    cout << endl;


    ifstream cleanedFile("test_cleaned.cpp");
    if (!cleanedFile.is_open()) {
        cout << "Error: Cannot open cleaned file." << endl;
        return 1;
    }

    stringstream buffer;
    buffer << cleanedFile.rdbuf();
    string sourceCode = buffer.str();
    cleanedFile.close();

    Lexer lexer(sourceCode);
    lexer.analyze();

    if (lexer.hasErrors()) {
        hasError = true;
        cout << "Lexical analysis failed with errors:" << endl;
        lexer.printErrors();
        cout << "Compilation terminated." << endl;
        return 1;
    }

    vector<Token> tokens = lexer.getTokens();
    cout << "  Tokens generated: " << tokens.size() << endl;

    // Сохраняем токены в файл
    ofstream tokenFile("tokens_output.txt");
    tokenFile << "=== TOKENS SEQUENCE ===\n[";
    for (size_t i = 0; i < tokens.size(); i++) {
        if (tokens[i].type == END_OF_FILE) continue;

        string typeStr;
        switch (tokens[i].type) {
        case KEYWORD: typeStr = "KEYWORD"; break;
        case IDENTIFIER: typeStr = "IDENTIFIER"; break;
        case OPERATOR: typeStr = "OPERATOR"; break;
        case DELIMITER: typeStr = "DELIMITER"; break;
        case CONSTANT_INT: typeStr = "CONSTANT_INT"; break;
        case CONSTANT_FLOAT: typeStr = "CONSTANT_FLOAT"; break;
        case CONSTANT_STRING: typeStr = "CONSTANT_STRING"; break;
        case CONSTANT_BOOL: typeStr = "CONSTANT_BOOL"; break;
        default: typeStr = "UNKNOWN";
        }
        tokenFile << "(" << typeStr << ", " << tokens[i].value << ")";
        if (i < tokens.size() - 2) tokenFile << " ";
    }
    tokenFile << "]\n(END_OF_FILE)\n";
    tokenFile.close();


    SemanticAnalyzer analyzer(tokens);
    analyzer.analyze();

    if (analyzer.hasErrors()) {
        hasError = true;
    }

    if (!hasError) {
        cout << "   COMPILATION COMPLETED SUCCESSFULLY" << endl;
    }
    else {
        cout << "   COMPILATION COMPLETED WITH ERRORS" << endl;
    }


    return hasError ? 1 : 0;
}