#include <iostream>
#include <string>
using namespace std;

int a = 10, b = 20, result = 0;

/*
   Multi-line comment
*/

int add(int x, int y) {
    return x + y;
}

bool isEven(int n) {
    return n % 2 == 0;
}

int main() {
    result = add(a, b);
    cout << "Result: " << result << endl;

    if (result > 25)
        cout << "Greater" << endl;
    else
        cout << "Less or equal" << endl;

    for (int i = 0; i < 3; i++)
        cout << "i = " << i << endl;

    int counter = 0;
    while (counter < 2) {
        cout << "counter = " << counter << endl;
        counter++;
    }

    /*
       Nested comment
    */
    cout << "End" << endl;
    return 0;
}