#include <iostream>
#include <signal.h>
#include "signals.h"
#include "Commands.h"
#include <wait.h>
using namespace std;

void ctrlZHandler(int sig_num) {
    cout << "smash: got ctrl-Z" << endl;
    SmallShell& smash = SmallShell::getInstance();
    // checks if there is an external command running in the fg:
    if(smash.getRunningCmd() != NO_RUNNING_CMD) {
        int status;
        status = waitpid(smash.getRunningCmd(), nullptr, WNOHANG);
        // if status == -1  <--> process died
        if(status == -1 or status == smash.getRunningCmd()) {
            return;
        }
        if(kill(smash.getRunningCmd(),SIGSTOP) == -1) {
            perror("smash error: kill failed");
            return;
        }
        cout << "smash: process " << smash.getRunningCmd() << " was stopped" << endl;
        smash.jobs.StopFG();
    }
    smash.jobs.setForeGroundJob(nullptr);
}

void ctrlCHandler(int sig_num) {
    cout << "smash: got ctrl-C" << endl;
    SmallShell& smash = SmallShell::getInstance();
    // checks if there is an external command running:
    if(smash.getRunningCmd() != NO_RUNNING_CMD) {
        int status;
        status = waitpid(smash.getRunningCmd(), nullptr, WNOHANG);
        // if status == pid <--> process exited (finished)
        if(status == -1 or status == smash.getRunningCmd()) {
            return;
        }
        if (kill(smash.getRunningCmd(),SIGKILL) == -1) {
            perror("smash error: kill failed");
            return;
        }
        cout << "smash: process " << smash.getRunningCmd() << " was killed" << endl;
    }
    smash.jobs.setForeGroundJob(nullptr);
}

void alarmHandler(int sig_num) {
    cout << "smash got an alarm" << endl;
//    SmallShell& smash = SmallShell::getInstance();
    pid_t alarm_sender = 1;     // todo change this!
    // todo check who sent the alaram????
    if( kill(alarm_sender,SIGKILL) == -1) {
        perror("smash error: kill failed");
    }
    cout << "smash" << "" << "timed out!" << endl;
    //TODO: update jobs list?

}

