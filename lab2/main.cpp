#include "lexer.h"
#include <iostream>
#include <fstream>
#include <sstream>

using namespace std;

int main(int argc, char* argv[]) {
    string inputFile = "test_cleaned.cpp";
    string outputFile = "tokens_output.txt";

    if (argc >= 2) {
        inputFile = argv[1];
    }
    if (argc >= 3) {
        outputFile = argv[2];
    }

    cout << "=== Lexical Analyzer ===" << endl;
    cout << "Input file: " << inputFile << endl;
    cout << "Output file: " << outputFile << endl;
    cout << endl;

    ifstream file(inputFile);
    if (!file.is_open()) {
        cout << "Error: cannot open input file " << inputFile << endl;
        cout << "Please run preprocessor first (Lab 1)" << endl;
        return 1;
    }

    stringstream buffer;
    buffer << file.rdbuf();
    string sourceCode = buffer.str();
    file.close();

    Lexer lexer(sourceCode);
    lexer.analyze();

    lexer.printTokens();
    lexer.printErrors();

    ofstream outFile(outputFile);
    if (outFile.is_open()) {
        outFile << "=== TOKENS SEQUENCE ===" << endl;
        outFile << "[( ";

        for (const auto& token : lexer.getTokens()) {
            if (token.type == TokenType::END_OF_FILE) {
                outFile << "(END_OF_FILE)";
                break;
            }

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
            outFile << "(" << typeStr << ", " << token.value << ") ";
        }
        outFile << "]";
        outFile.close();
    }

    cout << "\nLexical analysis completed." << endl;
    cout << "Tokens found: " << lexer.getTokens().size() - 1 << endl;

    if (lexer.hasErrors()) {
        cout << "Analysis completed with errors." << endl;
        return 1;
    }

    cout << "Analysis completed successfully." << endl;
    return 0;
}