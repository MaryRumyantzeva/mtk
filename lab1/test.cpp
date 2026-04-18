#include <iostream>
#include <string>

// Глобальные переменные
int a = 10;
int b = 20;
int result = 0;

/*
   Многострочный комментарий
   с описанием программы
   для тестирования препроцессора
*/

// Функция для сложения двух чисел
int add(int x, int y) {
    int sum = x + y;     // арифметическое выражение
    return sum;
}

// Функция для проверки чётности
bool isEven(int number) {
    if (number % 2 == 0) {      // логическое выражение
        return true;             // чётное
    }
    else {
        return false;            // нечётное
    }
}

int main() {
    // Объявление локальных переменных
    int i = 0;
    int counter = 0;

    std::cout << "Начало программы" << std::endl;     // вывод сообщения

    // Вызов функции сложения
    result = add(a, b);
    std::cout << "Результат сложения: " << result << std::endl;

    // Условный оператор if-else
    if (result > 25) {
        std::cout << "Результат больше 25" << std::endl;
    }
    else {
        std::cout << "Результат меньше или равен 25" << std::endl;
    }

    // Проверка чётности
    if (isEven(result)) {
        std::cout << "Результат - чётное число" << std::endl;
    }
    else {
        std::cout << "Результат - нечётное число" << std::endl;
    }

    // Цикл for
    for (i = 0; i < 5; i++) {
        std::cout << "i = " << i << std::endl;     // вывод счётчика
    }

    // Цикл while
    counter = 0;
    while (counter < 3) {
        std::cout << "counter = " << counter << std::endl;
        counter++;          // инкремент
    }

    /*
       Вложенный комментарий
       внутри основной программы
       который должен быть корректно удалён
    */

    std::cout << "Конец программы" << std::endl;
    return 0;
}