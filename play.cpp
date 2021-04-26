
#include <unistd.h>
#include <string.h>
#include <iostream>
#include <vector>
#include <sstream>
#include <sys/wait.h>
#include <time.h>
#include <iomanip>
#include "Commands.h"
#include <climits>
#include <wait.h>
using namespace std;


int main() {
    int* num = new int;
    *num = 1;
    pid_t p = fork();
    if(p == 0 ) {
        *num = 16;
        cout << "child num is at:  " << num << endl;
        cout << "updated num" << endl;
        delete num;
    }
    else {
        wait(nullptr);
        cout << "father num is at:  " << num << endl;
        cout << "num is " << *num << endl;
        delete num;
    }
}