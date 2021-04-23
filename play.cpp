
#include <unistd.h>
#include <string.h>
#include <iostream>
#include <vector>
#include <sstream>
#include <sys/wait.h>
#include <iomanip>
#include "Commands.h"
#include <climits>

using namespace std;


int main() {
    cout << "bash cmd:" << endl;
    execl("/bin/bash", "/bin/bash", "-c", "pwd", NULL);
}