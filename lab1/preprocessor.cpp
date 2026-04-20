#include <iostream>
#include <fstream>
#include <string>
#include <regex>
#include <vector>
#include <sstream>

using namespace std;

class Preprocessor {
private:
    string inputFileName;
    string outputFileName;

    int countOccurrences(const string& text, const string& pattern) {
        int count = 0;
        size_t pos = 0;
        while ((pos = text.find(pattern, pos)) != string::npos) {
            count++;
            pos += pattern.length();
        }
        return count;
    }

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


    string removeSingleLineCommentsAlternative(const string& code) {
        regex singleLineComment("//.*$", regex::ECMAScript);

        string result;
        size_t start = 0;
        size_t end = 0;

        while (end < code.length()) {
            size_t commentStart = code.find("//", start);

            if (commentStart == string::npos) {
                result += code.substr(start);
                break;
            }
            size_t lineEnd = code.find("\n", commentStart);
            if (lineEnd == string::npos) {
                lineEnd = code.length();
            }

            result += code.substr(start, commentStart - start);

            start = lineEnd;
            if (start < code.length() && code[start] == '\n') {
                result += "\n";
                start++;
            }
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
            cout << "Error: cannot open input file " << inputFileName << endl;
            return false;
        }

        stringstream buffer;
        buffer << inputFile.rdbuf();
        string code = buffer.str();
        inputFile.close();

        code = removeBOM(code);

        int openComments = countOccurrences(code, "/*");
        int closeComments = countOccurrences(code, "*/");

        if (openComments != closeComments) {
            cout << "Error: unclosed multi-line comment" << endl;
            cout << "Number of /*: " << openComments << ", */: " << closeComments << endl;
            cout << "File will not be saved." << endl;
            return false;
        }

       
        code = removeMultiLineComments(code);

        
        code = removeSingleLineComments(code);

        
        vector<string> cleanedLines = normalizeWhitespace(code);

        ofstream outputFile(outputFileName);
        if (!outputFile.is_open()) {
            cout << "Error: cannot create output file " << outputFileName << endl;
            return false;
        }

        for (const auto& line : cleanedLines) {
            outputFile << line << endl;
        }
        outputFile.close();

        cout << "File successfully processed." << endl;
        cout << "Result saved to: " << outputFileName << endl;
        cout << "No errors detected." << endl;

        return true;
    }
};

int main(int argc, char* argv[]) {
    string inputFile = "test.cpp";
    string outputFile = "test_cleaned.cpp";

    if (argc >= 2) {
        inputFile = argv[1];
    }
    if (argc >= 3) {
        outputFile = argv[2];
    }

    cout << "=== Preprocessor for Source Code Cleaning ===" << endl;
    cout << "Input file: " << inputFile << endl;
    cout << "Output file: " << outputFile << endl;
    cout << endl;

    Preprocessor preprocessor(inputFile, outputFile);

    if (!preprocessor.process()) {
        cout << "Processing completed with errors." << endl;
        return 1;
    }

    cout << endl << "Processing completed successfully." << endl;
    return 0;
}