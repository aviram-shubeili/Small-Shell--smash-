#include <iostream>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include "Commands.h"
#include "signals.h"

int main(int argc, char* argv[]) {
    if(signal(SIGTSTP , ctrlZHandler)==SIG_ERR) {
        perror("smash error: failed to set ctrl-Z handler");
    }
    if(signal(SIGINT , ctrlCHandler)==SIG_ERR) {
        perror("smash error: failed to set ctrl-C handler");
    }

    //TODO: setup sig alarm handler

    SmallShell& smash = SmallShell::getInstance();
    SmashOperation quit_flag = CONTINUE;
    while(quit_flag != QUIT) {
        std::cout << smash.getPromptLine() << "> ";
        std::string cmd_line;
        std::getline(std::cin, cmd_line);
        quit_flag = smash.executeCommand(cmd_line.c_str());

        // TODO free the array?
    }
    return 0;
}