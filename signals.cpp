#include <iostream>
#include <signal.h>
#include "signals.h"
#include "Commands.h"

using namespace std;

void ctrlZHandler(int sig_num) {
    cout << "smash: got ctrl-Z" << endl;
    SmallShell& smash = SmallShell::getInstance();
    // checks if there is an external command running:
    if(smash.getRunningCmd() != NO_RUNNING_CMD) {
        kill(smash.getRunningCmd(),SIGSTOP);
        cout << "smash: process " << smash.getRunningCmd() << " was stopped\n";
        smash.setRunningCmd(NO_RUNNING_CMD);
    }
}

void ctrlCHandler(int sig_num) {
    cout << "smash: got ctrl-C" << endl;
    SmallShell& smash = SmallShell::getInstance();
    // checks if there is an external command running:
    if(smash.getRunningCmd() != NO_RUNNING_CMD) {
        kill(smash.getRunningCmd(),SIGKILL);
        cout << "smash: process " << smash.getRunningCmd() << " was killed\n";
        smash.setRunningCmd(NO_RUNNING_CMD);
    }

}

void alarmHandler(int sig_num) {
    cout << "smash got an alarm" << endl;
//    SmallShell& smash = SmallShell::getInstance();
    pid_t alarm_sender = 1;     // todo change this!
    // todo check who sent the alaram????
    kill(alarm_sender,SIGKILL);
    cout << "smash" << "" << "timed out!" << endl;
}

