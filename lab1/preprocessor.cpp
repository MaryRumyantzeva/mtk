#include <iostream>
#include <fstream>
#include <string>
#include <regex>
#include <vector>
#include <sstream>

class Preprocessor {
private:
    std::string inputFileName;
    std::string outputFileName;

    // Подсчёт вхождений подстроки
    int countOccurrences(const std::string& text, const std::string& pattern) {
        int count = 0;
        size_t pos = 0;

        while ((pos = text.find(pattern, pos)) != std::string::npos) {
            count++;
            pos += pattern.length();
        }

        return count;
    }

    // Проверка на недопустимые символы
    bool hasInvalidCharacters(const std::string& text) {
        // Допустимые символы: буквы, цифры, пробелы, знаки пунктуации, операторы
        std::regex validChars("[a-zA-Z0-9\\s\\t\\n\\r\\{\\}\\(\\)\\[\\]<>;:,\\.\\+\\-\\*/%=!&|\\^~\"'#]");

        for (char c : text) {
            std::string charStr(1, c);
            if (!std::regex_match(charStr, validChars)) {
                std::cout << "Ошибка: обнаружен недопустимый символ '" << c
                    << "' (код: " << (int)c << ")" << std::endl;
                return true;
            }
        }
        return false;
    }

    // Удаление многострочных комментариев
    std::string removeMultiLineComments(const std::string& code) {
        // Регулярное выражение для многострочных комментариев
        // std::regex::ECMAScript | std::regex::optimize для производительности
        std::regex multiLineComment("/\\*.*?\\*/", std::regex::ECMAScript);
        return std::regex_replace(code, multiLineComment, "");
    }

    // Удаление однострочных комментариев
    std::string removeSingleLineComments(const std::string& code) {
        // Регулярное выражение для однострочных комментариев
        std::regex singleLineComment("//.*$", std::regex::ECMAScript | std::regex::multiline);
        return std::regex_replace(code, singleLineComment, "");
    }

    // Нормализация пробелов и удаление пустых строк
    std::vector<std::string> normalizeWhitespace(const std::string& code) {
        std::vector<std::string> result;
        std::istringstream stream(code);
        std::string line;

        while (std::getline(stream, line)) {
            // Замена последовательностей пробельных символов на один пробел
            std::regex whitespace("\\s+");
            std::string normalized = std::regex_replace(line, whitespace, " ");

            // Удаление пробелов в начале и конце строки
            normalized = std::regex_replace(normalized, std::regex("^\\s+|\\s+$"), "");

            // Добавляем только непустые строки
            if (!normalized.empty()) {
                result.push_back(normalized);
            }
        }

        return result;
    }

public:
    Preprocessor(const std::string& input, const std::string& output)
        : inputFileName(input), outputFileName(output) {}

    bool process() {
        // Чтение входного файла
        std::ifstream inputFile(inputFileName);
        if (!inputFile.is_open()) {
            std::cout << "Ошибка: не удалось открыть входной файл " << inputFileName << std::endl;
            return false;
        }

        std::stringstream buffer;
        buffer << inputFile.rdbuf();
        std::string code = buffer.str();
        inputFile.close();

        // Проверка на наличие ошибок
        bool hasError = false;

        // 1. Проверка незакрытых многострочных комментариев
        int openComments = countOccurrences(code, "/*");
        int closeComments = countOccurrences(code, "*/");

        if (openComments != closeComments) {
            std::cout << "Ошибка: незакрытый многострочный комментарий" << std::endl;
            std::cout << "Количество /*: " << openComments << ", */: " << closeComments << std::endl;
            std::cout << "Файл не будет сохранён." << std::endl;
            return false;
        }

        // 2. Проверка на недопустимые символы
        if (hasInvalidCharacters(code)) {
            std::cout << "Ошибка: обнаружены недопустимые символы" << std::endl;
            std::cout << "Файл не будет сохранён." << std::endl;
            return false;
        }

        // 3. Удаление многострочных комментариев
        code = removeMultiLineComments(code);

        // 4. Удаление однострочных комментариев
        code = removeSingleLineComments(code);

        // 5. Нормализация пробелов и удаление пустых строк
        std::vector<std::string> cleanedLines = normalizeWhitespace(code);

        // Запись результата
        std::ofstream outputFile(outputFileName);
        if (!outputFile.is_open()) {
            std::cout << "Ошибка: не удалось создать выходной файл " << outputFileName << std::endl;
            return false;
        }

        for (const auto& line : cleanedLines) {
            outputFile << line << std::endl;
        }
        outputFile.close();

        // Вывод информационных сообщений
        std::cout << "Файл успешно обработан." << std::endl;
        std::cout << "Результат сохранён в: " << outputFileName << std::endl;
        std::cout << "Удалено комментариев: " << (openComments + closeComments) << std::endl;
        std::cout << "Исходный размер: " << code.length() << " символов" << std::endl;
        std::cout << "Ошибок не выявлено" << std::endl;

        return true;
    }
};

int main(int argc, char* argv[]) {
    // Настройка имён файлов
    std::string inputFile = "test.cpp";
    std::string outputFile = "test_cleaned.cpp";

    // Поддержка аргументов командной строки
    if (argc >= 2) {
        inputFile = argv[1];
    }
    if (argc >= 3) {
        outputFile = argv[2];
    }

    std::cout << "=== Препроцессор для очистки исходного кода ===" << std::endl;
    std::cout << "Входной файл: " << inputFile << std::endl;
    std::cout << "Выходной файл: " << outputFile << std::endl;
    std::cout << std::endl;

    Preprocessor preprocessor(inputFile, outputFile);

    if (!preprocessor.process()) {
        std::cout << "Обработка завершена с ошибками." << std::endl;
        return 1;
    }

    std::cout << std::endl << "Обработка успешно завершена." << std::endl;
    return 0;
}