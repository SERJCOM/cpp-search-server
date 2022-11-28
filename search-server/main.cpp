// Решите загадку: Сколько чисел от 1 до 1000 содержат как минимум одну цифру 3?
// Напишите ответ здесь:

// Закомитьте изменения и отправьте их в свой репозиторий.


#include <iostream>

using namespace std;

bool sod(int a){
    bool b = false;
    while(a > 0){
        if(a % 10 == 3) {
            b = true;
            break;
        }
        a = a / 10;
    }
    return b;
}

int main()
{
    int count = 0;
    for(int i = 0; i < 1000; i++)
        if(sod(i)) count ++;
    cout << count;
    return 0;
}
