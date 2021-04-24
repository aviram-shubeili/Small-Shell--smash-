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

#if 0
#define FUNC_ENTRY()  \
  cout << __PRETTY_FUNCTION__ << " --> " << endl;

#define FUNC_EXIT()  \
  cout << __PRETTY_FUNCTION__ << " <-- " << endl;
#else
#define FUNC_ENTRY()
#define FUNC_EXIT()
#endif

string _ltrim(const std::string& s)
{
    size_t start = s.find_first_not_of(WHITESPACE);
    return (start == std::string::npos) ? "" : s.substr(start);
}

string _rtrim(const std::string& s)
{
    size_t end = s.find_last_not_of(WHITESPACE);
    return (end == std::string::npos) ? "" : s.substr(0, end + 1);
}

string _trim(const std::string& s)
{
    return _rtrim(_ltrim(s));
}

int _parseCommandLine(const char* cmd_line, char** args) {
    FUNC_ENTRY()
    int i = 0;
    std::istringstream iss(_trim(string(cmd_line)).c_str());
    for(std::string s; iss >> s; ) {
        args[i] = (char*)malloc(s.length()+1);
        memset(args[i], 0, s.length()+1);
        strcpy(args[i], s.c_str());
        args[++i] = NULL;
    }
    return i;

    FUNC_EXIT()
}

bool _isBackgroundCommand(const char* cmd_line) {
    const string str(cmd_line);
    size_t idx = str.find_last_not_of(WHITESPACE);
    return idx == string::npos ? false : str[idx] == '&';
}

void _removeBackgroundSign(string& cmd_line) {
    const string str(cmd_line);
    // find last character other than spaces
    unsigned int idx = str.find_last_not_of(WHITESPACE);
    // if all characters are spaces then return
    if (idx == string::npos) {
        return;
    }
    // if the command line does not end with & then return
    if (cmd_line[idx] != '&') {
        return;
    }
    // replace the & (background sign) with space and then remove all tailing spaces.
    cmd_line[idx] = ' ';
    // truncate the command line string up to the last non-space character
    cmd_line[str.find_last_not_of(WHITESPACE, idx) + 1] = 0;
}

// TODO: Add your implementation for classes in Commands.h 

SmallShell::SmallShell() : prompt_line("smash")
{
// TODO: add your implementation
}

SmallShell::~SmallShell() {
// TODO: add your implementation
}

/**
* Creates and returns a pointer to Command class which matches the given command line (cmd_line)
*/
Command * SmallShell::CreateCommand(const char* cmd_line) {

    string cmd_s = _trim(string(cmd_line));
    string firstWord = cmd_s.substr(0, cmd_s.find_first_of(" \n"));

    if (firstWord.compare("pwd") == 0){
        return new GetCurrDirCommand(cmd_line);
    }
    else if (firstWord.compare("showpid") == 0){
        return new ShowPidCommand(cmd_line);
    }
    else if (firstWord.compare("chprompt")==0){
        return new ChangePromptCommand(cmd_line, &prompt_line);
    }
    else if (firstWord.compare("cd")==0){
        return new ChangeDirCommand(cmd_line, &last_working_directory); // TODO fix
    }
    else {
        return new ExternalCommand(cmd_line);
    }
/*
    else if (firstWord.compare("jobs")==0){
        return new JobsList(cmd_line); // external
    }
    else if (firstWord.compare("kill")==0){
        return new KillCommand(cmd_line); // external
    }
    else if (firstWord.compare("fg")==0){
        return new ForegroundCommand(cmd_line); // external
    }
    else if (firstWord.compare("bg")==0){
        return new BackgroundCommand(cmd_line); // external
    }
    else if (firstWord.compare("quit")==0){
        return new QuitCommand(cmd_line); // external
    }
     */

    /*
	// For example:

  string cmd_s = _trim(string(cmd_line));
  string firstWord = cmd_s.substr(0, cmd_s.find_first_of(" \n"));

  if (firstWord.compare("pwd") == 0) {
    return new GetCurrDirCommand(cmd_line);
  }
  else if (firstWord.compare("showpid") == 0) {
    return new ShowPidCommand(cmd_line);
  }
  else if ...
  .....
  else {
    return new ExternalCommand(cmd_line);
  }
*/
    return nullptr;
}

void SmallShell::executeCommand(const char *cmd_line) {
    Command *cmd = CreateCommand(cmd_line);
    if (typeid(*cmd)==typeid(ExternalCommand))
    {
        pid_t p = fork();
        if (p == -1) {
            // fork failed
            perror("smash: fork failed");
        }
        else if (p == 0){
            // child code
            setpgrp();
            cmd->execute();
        }
        else{
            // father code
            running_cmd = p;
            if(not (dynamic_cast<ExternalCommand*>(cmd)->is_bg_cmd)) {
                // if its a foreground command than wait for it to end/stop
                waitpid(running_cmd,nullptr, WUNTRACED);
            }
        }
    }
    else {
        //if (cmd_line[0]=="") { // TODO handle forking external commands
        cmd->execute();
    }
    //}
    // todo should i check if we need to??
    delete cmd;
    // TODO: Add your implementation here
    // for example:
    // Command* cmd = CreateCommand(cmd_line);
    // cmd->execute();
    // Please note that you must fork smash process for some commands (e.g., external commands....)
}

const string &SmallShell::getPromptLine() const {
    return prompt_line;
}

pid_t SmallShell::getRunningCmd() const {
    return running_cmd;
}

void SmallShell::setRunningCmd(pid_t runningCmd) {
    running_cmd = runningCmd;
}


//==================================== Commands ======================================//


BuiltInCommand::BuiltInCommand(const char *cmd_line) : Command(cmd_line) {
    string temp(cmd_line);
    // ignore & sign
    _removeBackgroundSign(temp);
    num_arg = _parseCommandLine(temp.c_str(), arguments);
}

BuiltInCommand::~BuiltInCommand() {
    // freeilsng malloc made by _parseCommandLine (should automaticly be done by ~Command)
    for(int i = 0 ; i < num_arg ;++i) {
        free(arguments[i]);
    }
}

ExternalCommand::ExternalCommand(const char *cmd_line) : Command(cmd_line),
                                                         bash_cmd(cmd_line) {
    is_bg_cmd = _isBackgroundCommand(cmd_line);
    if(is_bg_cmd) {
        _removeBackgroundSign(bash_cmd);
    }

}

ExternalCommand::~ExternalCommand()  {}

void ShowPidCommand::execute() {
    pid_t pid = getpid();
    cout << "smash pid is "<< pid << "\n";
}

void GetCurrDirCommand::execute() {
    char buff[PATH_MAX];
    cout << getcwd(buff, PATH_MAX) << endl;
}


void ChangeDirCommand::execute() {
    std::string path;
    // no arguments - does nothing
    if(num_arg == 1) {
        return;
    }
    // too many args
    if (num_arg != 2) {
        cerr << "smash error: cd: too many arguments\n";
        return;
    }
        // 2 args :]
    else {
        // go back to old pwd
        if(strcmp(arguments[1], "-") == 0) {
            // no where to go back
            if(oldpwd->empty()) {
                cerr << "smash error: cd: OLDPWD not set\n";
                return;
            }
            else {
                path = *oldpwd;
            }
        }
        else {
            path = arguments[PATH_ARG];
        }
        // saving oldpwd
        char buff[PATH_MAX];
        getcwd(buff, PATH_MAX);
        // executing with feedback from syscall
        if(chdir(path.c_str()) == -1) {
            perror("smash error: chdir failed");
            return;
        }
        // if chdir succeeded - update oldpwd
        *oldpwd = buff;

    }
}

ChangeDirCommand::ChangeDirCommand(const char *cmd_line, string *oldpwd) : BuiltInCommand(cmd_line),
                                                                           oldpwd(oldpwd) {}


ChangePromptCommand::ChangePromptCommand(const char *cmd_line, std::string *smash_prompt) : BuiltInCommand(cmd_line),
                                                                                            smash_prompt(smash_prompt) {}

void ChangePromptCommand::execute() {
    // resetting prompt
    if(num_arg == 1) {
        *smash_prompt = "smash";
    }
        // setting new prompt
    else {
        *smash_prompt = arguments[1];
    }
}

void ExternalCommand::execute() {
    execl("/bin/bash", "/bin/bash", "-c", bash_cmd.c_str(), NULL);
}


//=========================================Jobs List===========================================//

JobsList::JobsList() : job_arr(new JobEntry* [MAX_JOBS]), next_id(1), to_clear(std::vector<int>(NULL)) {
    //TODO loop to init arr to nullptrs or is this enough?
}

void JobsList::addJob(Command* cmd, bool isStopped){
    job_arr[next_id] = new JobEntry(next_id, cmd);
    for (int i=next_id+1; i<MAX_JOBS; i++){
        if (job_arr[i] == nullptr) {
            next_id = i;
            break;
        }
    }
}

void JobsList::printJobsList(){
    for (int i=0; i<MAX_JOBS; i++){
        if (job_arr[i]) {
            std::cout << "[" << job_arr[i]->getJobId() << "] ";
            std::cout << job_arr[i]->getCommand() << " : ";
            std::cout << getpid() << " "; // TODO how to get pid of a command?
            std::cout << difftime(time(nullptr), *(job_arr[i]->getTime())) << " secs ";
            if (job_arr[i]->isStopped()){
                std::cout << "(stopped)";
            }
            std::cout << std::endl;
        }
    }
}

void JobsList::killAllJobs(){
    SmallShell& smash = SmallShell::getInstance();
    for (int i=0; i<MAX_JOBS; i++){
        if (job_arr[i]){
            //TODO send kill signal to job_arr[i]->Command
        }
    }
}

void JobsList::removeFinishedJobs(){
    for (std::vector<int>::iterator it = this->to_clear.begin(); it != this->to_clear.end(); ++it){
        if ((*it)<this->next_id) { this->next_id = *it; } // updates the new lowest free job id
        delete (job_arr[(*it)]);
        this->job_arr[(*it)] = nullptr;
        //TODO do we need to do additional operations?
    }
    to_clear.clear();
}
JobsList::JobEntry* JobsList::getJobById(int jobId){
    return this->job_arr[jobId];
}
void JobsList::removeJobById(int jobId){
    for (std::vector<int>::iterator it = this->to_clear.begin(); it != this->to_clear.end(); ++it){
        if (*it == jobId) {
            to_clear.erase(it);
            break;
        }
    }
    delete(job_arr[jobId]);
    this->job_arr[jobId] = nullptr;
    if (jobId < this->next_id) { this->next_id = jobId; }
}
JobsList::JobEntry* JobsList::getLastJob(int* lastJobId){} // TODO last job that was done? or something else?
JobsList::JobEntry* JobsList::getLastStoppedJob(int *jobId){}