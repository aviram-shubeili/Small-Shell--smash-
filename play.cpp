
#include <unistd.h>
#include <string>
#include <iostream>
#include <vector>
#include <sstream>
#include <sys/wait.h>
#include <time.h>
#include <iomanip>
#include "Commands.h"
#include <climits>
#include <wait.h>
#include <algorithm>

using namespace std;
bool isNumber(const std::string& s)
{
    return !s.empty() &&
           std::find_if(s.begin(),
                        s.end(), [](unsigned char c) { return !std::isdigit(c); }) == s.end();
}

int main() {

    std::string str = "-3";
    bool isNum = isNumber(str);
    if(isNum) {
        int num = stoi(str);
    cout << isNum << endl << num << endl;
    }


    for(int i = 0 ; i < 30 ; i++) {
        std::cout << i << endl;
        sleep(1);
    }
                  int status;
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
        sleep(5);
        kill(p,SIGCONT);
        cout << "father num is at:  " << num << endl;
        cout << "num is " << *num << endl;
        delete num;
    }
}