#include <unistd.h>
#include <string.h>
#include <iostream>
#include <memory>
#include <vector>
#include <sstream>
#include <sys/wait.h>
#include <iomanip>
#include "Commands.h"
#include <climits>
#include <assert.h>
#include <algorithm>

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

SpecialCommand _identifyAndSeperateSpecialSigns(const char* cmd_line, std::string& result) {
    std::string cmd_s(cmd_line);
    size_t sign_pos;
    // search for ">>"
    sign_pos = cmd_s.find(">>");
    if(sign_pos != string::npos) {
        result = cmd_s.substr(0,sign_pos);
        result += " >> ";
        result += cmd_s.substr(sign_pos + 2);
        return REDIRECTION_APPEND;
    }
    // search for ">"
    sign_pos = cmd_s.find(">");
    if(sign_pos != string::npos) {
        result = cmd_s.substr(0,sign_pos);
        result += " > ";
        result += cmd_s.substr(sign_pos + 1);
        return REDIRECTION;
    }
    // search for "|&"
    sign_pos = cmd_s.find("|&");
    if(sign_pos != string::npos) {
        result = cmd_s.substr(0,sign_pos);
        result += " |& ";
        result += cmd_s.substr(sign_pos + 2);
        return PIPE_TO_ERR;
    }
    // search for "|"
    sign_pos = cmd_s.find("|");
    if(sign_pos != string::npos) {
        result = cmd_s.substr(0,sign_pos);
        result += " | ";
        result += cmd_s.substr(sign_pos + 1);
        return PIPE;
    }
    // no special signs
    result = cmd_s;
    return NORMAL;
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

bool isNumber(const std::string& s)
{
    if(s.empty() || ((!isdigit(s[0])) && (s[0] != '-') && (s[0] != '+'))) return false;

    char * p;
    strtol(s.c_str(), &p, 10);

    return (*p == 0);
}



SmallShell::SmallShell() : prompt_line("smash"), external_quit_flag(false), smash_pid(getpid()){}

SmallShell::~SmallShell() {}

/**
* Creates and returns a pointer to Command class which matches the given command line (cmd_s)
*/
std::shared_ptr<Command> SmallShell::CreateCommand(const char* cmd_line) {

    string cmd_s = _trim(string(cmd_line));
    string firstWord = cmd_s.substr(0, cmd_s.find_first_of(" \n"));

    string special_cmd;
    switch (_identifyAndSeperateSpecialSigns(cmd_s.c_str(),special_cmd)) {
        case PIPE:
            return std::shared_ptr<Command>(new PipeCommand(special_cmd.c_str(), PIPE));
            break;
        case PIPE_TO_ERR:
            return std::shared_ptr<Command>(new PipeCommand(special_cmd.c_str(), PIPE_TO_ERR));
            break;
        case REDIRECTION:
            return std::shared_ptr<Command>(new RedirectionCommand(special_cmd.c_str(), REDIRECTION));
            break;
        case REDIRECTION_APPEND:
            return std::shared_ptr<Command>(new RedirectionCommand(special_cmd.c_str(), REDIRECTION_APPEND));
            break;
        case NORMAL:
            // not a special command --> move on
            break;
    }

    if (firstWord.compare("pwd") == 0 or firstWord.compare(("pwd&")) == 0){
        return std::shared_ptr<Command>(new GetCurrDirCommand(cmd_line));
    }
    else if (firstWord.compare("showpid") == 0 or firstWord.compare(("showpid&")) == 0){
        return std::shared_ptr<Command>(new ShowPidCommand(cmd_line));
    }
    else if (firstWord.compare("chprompt")==0 or firstWord.compare(("chprompt&")) == 0){
        return std::shared_ptr<Command>(new ChangePromptCommand(cmd_line, &prompt_line));
    }
    else if (firstWord.compare("cd")==0 or firstWord.compare(("cd&")) == 0){
        return std::shared_ptr<Command>(new ChangeDirCommand(cmd_line, &last_working_directory));
    }
    else if (firstWord.compare("jobs")==0 or firstWord.compare(("jobs&")) == 0){
        return std::shared_ptr<Command>(new JobsCommand(cmd_line,&jobs));
    }
    else if (firstWord.compare("kill")==0 or firstWord.compare(("kill&")) == 0){
        return std::shared_ptr<Command>(new KillCommand(cmd_line, &jobs));
    }
    else if (firstWord.compare("fg")==0 or firstWord.compare(("fg&")) == 0){
        return std::shared_ptr<Command>(new ForegroundCommand(cmd_line, &jobs));
    }
    else if (firstWord.compare("bg")==0 or firstWord.compare(("bg&")) == 0){
        return std::shared_ptr<Command>( new BackgroundCommand(cmd_line, &jobs));
    }
    else if (firstWord.compare("quit")==0 or firstWord.compare(("quit&")) == 0){
        external_quit_flag = true;
        return std::shared_ptr<Command>(new QuitCommand(cmd_line, &jobs));
    }
    else if (firstWord.compare("cat")==0 or firstWord.compare(("cat&")) == 0){
        return std::shared_ptr<Command>(new CatCommand(cmd_line));
    }
    else {
        return std::shared_ptr<Command>(new ExternalCommand(cmd_line));
    }

}

SmashOperation SmallShell::executeCommand(const char *cmd_line) {
    std::shared_ptr<Command> cmd = CreateCommand(cmd_line);

    // External commands special handling
    if (typeid(*cmd) == typeid(ExternalCommand))
    {
        pid_t p = fork();
        if (p == -1) {
            // fork failed
            perror("smash error: fork failed");
        }
        else if (p == 0){
            // child code
            setpgrp();
            cmd->execute();
        }
        else{
            // father code
            // updating son's pid
            cmd->setCmdPid(p);
            if(not (dynamic_cast<ExternalCommand*>(cmd.get())->is_bg_cmd)) {
                // if its a foreground command:
                jobs.setForeGroundJob(cmd);
                waitpid(jobs.getForeGroundJob()->getJobPid(),nullptr, WUNTRACED);
            }
            else {
                jobs.addJob(cmd);
            }
        }
    }
    else {
        cmd->execute();
    }

    if (typeid(*cmd) == typeid(QuitCommand)) {
        return QUIT;
    }
    else {
        return CONTINUE;
    }
}

const string &SmallShell::getPromptLine() const {
    return prompt_line;
}

pid_t SmallShell::getRunningCmd() const {
    if(jobs.getForeGroundJob()) {
        return jobs.getForeGroundJob()->getJobPid();
    }
    return NO_RUNNING_CMD;
}



//==================================== Commands ======================================//


BuiltInCommand::BuiltInCommand(const char *cmd_line) : Command(cmd_line) {
    string temp(this->cmd_line);
    // ignore & sign
    _removeBackgroundSign(temp);
    num_arg = _parseCommandLine(temp.c_str(), arguments);
}

BuiltInCommand::~BuiltInCommand() {
    // freeing malloc made by _parseCommandLine (should automaticly be done by ~Command)
    for(int i = 0 ; i < num_arg ;++i) {
        free(arguments[i]);
    }
}

ExternalCommand::ExternalCommand(const char *cmd_line) : Command(cmd_line),
                                                         bash_cmd(this->cmd_line)
{
    is_bg_cmd = _isBackgroundCommand(bash_cmd.c_str());
    if(is_bg_cmd) {
        _removeBackgroundSign(bash_cmd);
    }
}



void ShowPidCommand::execute() {
    SmallShell& smash = SmallShell::getInstance();
    cout << "smash pid is "<< smash.smash_pid << endl;
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
        cerr << "smash error: cd: too many arguments" << endl ;
        return;
    }
        // 2 args :]
    else {
        // go back to old pwd
        if(strcmp(arguments[1], "-") == 0) {
            // no where to go back
            if(oldpwd->empty()) {
                cerr << "smash error: cd: OLDPWD not set" << endl;
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

void Command::setCmdPid(pid_t cmdPid) {
    cmd_pid = cmdPid;
}

pid_t Command::getCmdPid() const {
    return cmd_pid;
}

ostream &operator<<(ostream &os, const Command &command) {
    os << command.untrimmed_cmd_line;
    return os;
}

Command::Command(const char *cmd_line)  : cmd_line(_trim(cmd_line)),
                                          untrimmed_cmd_line(cmd_line),
                                          cmd_pid(0) {}


//=========================================Jobs List===========================================//
//==============Job Entry==========//

JobsList::JobEntry::JobEntry
        (std::shared_ptr<Command> &cmd, pid_t p, bool is_stopped, int id) : cmd(cmd),
                                                                            is_stopped(is_stopped),
                                                                            job_pid(p),
                                                                            job_id(id)
{
    // update  executed time
    time(&time_executed);
}
void JobsList::JobEntry::setIsStopped(bool isStopped) {
    is_stopped = isStopped;
}

void JobsList::JobEntry::resetTime() {
    time(&time_executed);
}


JobsList::JobsList() : jobs(MAX_JOBS) {}

void JobsList::addJob(std::shared_ptr<Command> cmd, bool isStopped){
    int job_id = getMinFreeID();
    jobs[job_id] = std::make_shared<JobEntry>(cmd, cmd->getCmdPid(),false, job_id);
}

void JobsList::printJobsList(){
    removeFinishedJobs();
    for (int i = 1; i < MAX_JOBS; i++){
        if (jobs[i]) {
            std::cout << "[" << i << "] ";
            std::cout << *(jobs[i]->getCommand()) << " : ";
            std::cout << jobs[i]->getJobPid() << " ";
            std::cout << difftime(time(nullptr), jobs[i]->getTime()) << " secs ";
            if (jobs[i]->isStopped()){
                std::cout << "(stopped)";
            }
            std::cout << std::endl;
        }
    }
}

void JobsList::killAllJobs(){
    removeFinishedJobs();
    int jobs_count = 0;
    for(int i = 1 ; i < MAX_JOBS ; i++) {
        if(jobs[i]) {
            jobs_count++;
        }
    }
    cout << "smash: sending SIGKILL signal to " << jobs_count << " jobs:" << endl;
    for (int i = 1; i<MAX_JOBS; i++){
        if (jobs[i]){
            cout << jobs[i]->getJobPid() << ": " << *(jobs[i]->getCommand()) << endl;
            if(kill(jobs[i]->getJobPid(),SIGKILL) == -1) {
                perror("smash error: kill failed");
            }
            jobs[i] = nullptr;
        }
    }
}

void JobsList::removeFinishedJobs(){
    for (int i=0; i<MAX_JOBS; i++){
        if (jobs[i]){
            int status;
            status = waitpid(jobs[i]->getJobPid(), nullptr, WNOHANG);
            // if status == pid <--> process exited (finished)
            if(status == jobs[i]->getJobPid()) {
                jobs[i] = nullptr;
            }
        }
    }
}
shared_ptr<JobsList::JobEntry> JobsList::getJobById(int jobId){
    return jobs[jobId];
}
//  Warning! this doesnt kill process! only removes from list.
// user should have a pointer to job (via getJobById) before removing from list.
void JobsList::removeJobById(int jobId){
    if( 0 < jobId and jobId < MAX_JOBS) {
        jobs[jobId] = nullptr;
    }
}

shared_ptr<JobsList::JobEntry> JobsList::getLastStoppedJob(int *jobId) {
    removeFinishedJobs();
    for (int i = MAX_JOBS - 1; i > 0; --i) {
        if (jobs[i] and jobs[i]->isStopped()) {
            *jobId = i;
            return jobs[i];
        }
    }
    return nullptr;
}

int JobsList::getMinFreeID() {
    for (int i = 1; i < MAX_JOBS; i++) {
        if (jobs[i] == nullptr) {
            return i;
        }
    }
    // NOTE: should never get here!
    assert(false);
    return MAX_JOBS;

}

void JobsList::setForeGroundJob(std::shared_ptr<Command> fg_cmd) {
    if(fg_cmd == nullptr) {
        fg_job = nullptr;
    }
    else {
        fg_job = std::make_shared<JobEntry>(fg_cmd, fg_cmd->getCmdPid());
    }
}

const shared_ptr<JobsList::JobEntry> &JobsList::getForeGroundJob() const {
    return fg_job;
}

void JobsList::MarkStopped(int job_id) {
    if( 0 < job_id and job_id < MAX_JOBS and jobs[job_id]) {
        jobs[job_id]->setIsStopped(true);
    }
}


void JobsList::MarkCont(int job_id) {
    if( 0 < job_id and job_id < MAX_JOBS and jobs[job_id]) {
        jobs[job_id]->setIsStopped(false);
    }
}

void JobsList::StopFG() {
    if(fg_job) {
        fg_job->setIsStopped(true);
        fg_job->resetTime();
        jobs[getMinFreeID()] = fg_job;
        fg_job = nullptr;
    }
}

bool JobsList::isExists(int job_id) {
    return 0 < job_id and
           job_id < MAX_JOBS and
           jobs[job_id] != nullptr;
}

pid_t JobsList::getPIDByJobId(int jobId) {
    if(not isExists(jobId)) {
        return 0;
    }
    return jobs[jobId]->getJobPid();
}

int JobsList::getLastJobId(int *lastJobId) {
    for(int i = MAX_JOBS-1 ; i > 0 ; i--) {
        if(jobs[i]) {
            *lastJobId = i;
            return i;
        }
    }
    // if list is empty - return -1 to indicate.
    *lastJobId = -1;
    return -1;
}

void JobsList::moveBGToFG(int job_id) {
    if (not isExists(job_id)) {
        return;
    }
    // update FGJob and remove from list
    fg_job = jobs[job_id];
    jobs[job_id] = nullptr;
    std::cout << *(fg_job->getCommand()) << " : ";
    std::cout << fg_job->getJobPid() << " " << endl;
    if(kill(fg_job->getJobPid(), SIGCONT) == -1) {
        perror("smash error: kill failed");
    }
    // wait for the process to finish (or to be stopped by ctrl + z)
    waitpid(fg_job->getJobPid(), nullptr, WUNTRACED);
}

bool JobsList::isStopped(int job_id) {
    return isExists(job_id) and
           jobs[job_id]->isStopped();
}

void JobsList::ContinueJob(int job_id) {
    if(not isStopped(job_id)) {
        return;
    }
    std::cout << *(jobs[job_id]->getCommand()) << " : ";
    std::cout << jobs[job_id]->getJobPid() << " " << endl;
    MarkCont(job_id);
    if(kill(jobs[job_id]->getJobPid(), SIGCONT) == -1) {
        perror("smash error: kill failed");
    }
    // dont wait.
}


JobsCommand::JobsCommand(const char *cmd_line, JobsList *jobs) : BuiltInCommand(cmd_line),
                                                                 jobs(jobs) {
}
void JobsCommand::execute() {
    jobs->printJobsList();

}

KillCommand::KillCommand(const char *cmd_line, JobsList *jobs) : BuiltInCommand(cmd_line),
                                                                 jobs(jobs){

}
bool KillCommand::getArguments() {
    if(num_arg != KILL_CMD_ARG_NUM) {
        return false;
    }
    std::string signal_str = arguments[1];
    std::string job_str = arguments[2];
    // check if signal starts with a -
    if(signal_str[0] != '-') {
        return false;
    }
    signal_str = signal_str.substr(1);
    // check if the argument is a number
    if(not isNumber(signal_str) or not isNumber(job_str)) {
        return false;
    }
    // convert strings to int
    signal = stoi(signal_str);
    job_id = stoi(job_str);
    return true;
}
void KillCommand::execute() {
    if(num_arg != KILL_CMD_ARG_NUM or not getArguments()) {
        cerr << "smash error: kill: invalid arguments" << endl;
        return;
    }
    // make sure we dont send signal to a zombie process:
    jobs->removeFinishedJobs();

    if(not jobs->isExists(job_id)) {
        cerr << "smash error: kill: job-id " << job_id <<  " does not exist" << endl;
        return;
    }
    if(kill(jobs->getPIDByJobId(job_id),signal) == -1) {
        perror("smash error: kill failed");
        return;
    }

    cout << "signal number " << signal << " was sent to pid " << jobs->getPIDByJobId(job_id) << endl;
}


ForegroundCommand::ForegroundCommand(const char *cmd_line, JobsList *jobs) : BuiltInCommand(cmd_line),
                                                                             jobs(jobs) { }

bool ForegroundCommand::getArguments() {
    if( num_arg > 2) {
        return false;
    }
    if( num_arg == 2 and not isNumber(arguments[1])) {
        return false;
    }
    if( num_arg == 2 ) {
        job_id = stoi(arguments[1]);
    }
    else {
        job_id = jobs->getLastJobId(&job_id);
    }
    return true;
}

void ForegroundCommand::execute() {
    if(not getArguments()) {
        cerr << "smash error: fg: invalid arguments" << endl;
        return;
    }
    if(num_arg == 2) {
        if( not jobs->isExists(job_id)) {
            cerr << "smash error: fg: job-id" << job_id << " does not exist" << endl;
            return;
        }
    }
    else {
        // arg_num == 1
        if(job_id == -1) {
            // getLastJobId return -1 --> jobs list is empty
            cerr << "smash error: fg: jobs list is empty" << endl;
            return;
        }
    }
    jobs->moveBGToFG(job_id);
}

BackgroundCommand::BackgroundCommand(const char *cmd_line, JobsList *jobs) : BuiltInCommand(cmd_line),
                                                                             jobs(jobs) { }

bool BackgroundCommand::getArguments() {
    if( num_arg > 2) {
        return false;
    }
    if( num_arg == 2 and not isNumber(arguments[1])) {
        return false;
    }
    if( num_arg == 2 ) {
        job_id = stoi(arguments[1]);
    }
    else {
        jobs->getLastStoppedJob(&job_id);
    }
    return true;
}

void BackgroundCommand::execute() {
    if(not getArguments()) {
        cerr << "smash error: bg: invalid arguments" << endl;
        return;
    }
    if(num_arg == 2) {
        if( not jobs->isExists(job_id)) {
            cerr << "smash error: bg: job-id " << job_id << " does not exist" << endl;
            return;
        }
    }
    else if(num_arg == 1) {
        if(job_id == -1) {
            // getLastJobId return -1 --> jobs list is empty
            cerr << "smash error: bg: there is no stopped jobs to resume" << endl;
            return;
        }
    }
    else if(not jobs->isStopped(job_id)) {
        cerr << "smash error: bg: " << job_id << " is already running in the background" << endl;
        return;
    }

    jobs->ContinueJob(job_id);

}

QuitCommand::QuitCommand(const char* cmd_line, JobsList* jobs) : BuiltInCommand(cmd_line),
                                                                 jobs(jobs) {}

void QuitCommand::execute() {
    if(num_arg >= 2 and strcmp(arguments[1],"kill") == 0) {
        jobs->killAllJobs();
    }
}

PipeCommand::PipeCommand(const char *cmd_line, SpecialCommand op) :Command(cmd_line),
                                                                   op(op) {
    string temp(this->cmd_line);
    size_t op_pos;
    if(op == PIPE) {
        op_pos = temp.find("|");
        if(temp.find_last_not_of(WHITESPACE) == op_pos) {
            // no file name given // TODO do something!!
        }
        else {
            // skip | and space
            cmd2_s = temp.substr(op_pos + 2);
        }
        cmd1_s = temp.substr(0, op_pos);
    }
        // PIPE_TO_ERR
    else {
        op_pos = temp.find("|&");
        if(temp.find_last_not_of(WHITESPACE) == op_pos + 1) {
            // no file name given // TODO do something!!
        }
        else {
            // skip |& and space
            cmd2_s = temp.substr(op_pos + 3);
        }
        cmd1_s = temp.substr(0, op_pos);
    }

}

void PipeCommand::execute() {
    SmallShell& smash = SmallShell::getInstance();

    int fd [2];
    if (pipe(fd) == -1){
        perror("smash error: pipe failed");
        return;
    }

    pid_t p1 = fork();
    if (p1 == -1) {
        // fork failed
        perror("smash error: fork failed");
        return;
    }
    else if (p1 == 0){
        //  son 1 code
        setpgrp();
        if(op == PIPE) {
            if(dup2(fd[PIPE_WRITE],STDOUT_FD) == -1) {
                perror("smash error: dup2 failed");
                return;
            }
        }
        else {
            // op == PIPE_ERR
            if(dup2(fd[PIPE_WRITE],STDERR_FD) == -1) {
                perror("smash error: dup2 failed");
                return;
            }
        }
        DO_CLOSE(close(fd[PIPE_READ]));
        DO_CLOSE(close(fd[PIPE_WRITE]));
        smash.executeCommand(cmd1_s.c_str());
        smash.external_quit_flag = true;
    }
    else {
        //father code
        pid_t p2 = fork();
        if (p2 == -1) {
            // fork failed
            perror("smash error: fork failed");
            return;
        }
        else if (p2 == 0) {
            //  son 2 code
            setpgrp();
            if( dup2(fd[PIPE_READ],STDIN_FD) == -1) {
                perror("smash error: dup2 failed");
                return;
            }
            DO_CLOSE(close(fd[PIPE_READ]));
            DO_CLOSE(close(fd[PIPE_WRITE]));
            smash.executeCommand(cmd2_s.c_str());
            smash.external_quit_flag = true;
        }
        else {
            DO_CLOSE(close(fd[PIPE_READ]));
            DO_CLOSE(close(fd[PIPE_WRITE]));
            waitpid(p1,nullptr, WUNTRACED);
            waitpid(p2,nullptr, WUNTRACED);
        }
    }
}


RedirectionCommand::RedirectionCommand(const char *cmd_line, SpecialCommand op) : Command(cmd_line),
                                                                                  op(op) {
    string temp(this->cmd_line);
    size_t op_pos;
    if(op == REDIRECTION_APPEND) {
        op_pos = temp.find(">>");
        if(temp.find_last_not_of(WHITESPACE) == op_pos + 1) {
            // no file name given // TODO do something!!
        }
        else {
            // skip >> and space
            file_path = temp.substr(op_pos + 3);
        }
    }
        // REDIRECTION
    else {
        op_pos = temp.find(">");
        if(temp.find_last_not_of(WHITESPACE) == op_pos) {
            // no file name given // TODO do something!!
        }
        else {
            // skip > and space
            file_path = temp.substr(op_pos + 2);
        }
    }
    cmd_s = temp.substr(0,op_pos);
    file_path = _trim(file_path);
    cmd_s = _trim(cmd_s);
}

void RedirectionCommand::execute() {
    SmallShell &smash = SmallShell::getInstance();

    prepare();
    if(fd_num == -1) {
        return;
    }

    smash.executeCommand(cmd_s.c_str());
    cleanup();
}

void RedirectionCommand::prepare()  {

    temp_stdout_fd = dup(STDOUT_FD);
    if(temp_stdout_fd == -1) {
        perror("smash error: dup failed");
        return;
    }
    fd_num = (op == REDIRECTION_APPEND) ?
             open(file_path.c_str(),  O_APPEND | O_WRONLY | O_CREAT, 0666) :
             open(file_path.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if(fd_num == -1) {
        perror("smash error: open failed");
        return;
    }
    if(dup2(fd_num,STDOUT_FD) == -1) {

        if(close(fd_num) == -1) {
            perror("smash error: close failed");
            fd_num = -1;
            return;
        }
        perror("smash error: dup2 failed");
        fd_num = -1;
        return;
    }
}

void RedirectionCommand::cleanup() {

    if(close(fd_num) == -1) {
        perror("smash error: close failed");
        return;
    }
    if(dup2(temp_stdout_fd, STDOUT_FD) == -1) {
        perror("smash error: dup2 failed");
    }
    if(close(temp_stdout_fd) == -1) {
        perror("smash error: close failed");
        return;
    }
}

CatCommand::CatCommand(const char *cmd_line) : BuiltInCommand(cmd_line) {}

void CatCommand::execute() {

    if(num_arg == 1) {
        cerr << "smash error: cat: not enough arguments" << endl;
        return;
    }

    for(int i=1 ; i< num_arg ; i++) {
        int fd = open(arguments[i], O_RDONLY);
        if (fd == -1) {
            perror("smash error: open failed");
        } else {
            ssize_t bytes_read_successfully;
            char buffer[READING_ITERATION + 1];
            do {
                bytes_read_successfully = read(fd, buffer, READING_ITERATION);
                if (bytes_read_successfully == -1) {
                    perror("smash error: read failed");
                } else {
                    cout << buffer;
                }

            } while (bytes_read_successfully == READING_ITERATION);

            if (close(fd) == -1) {
                perror("smash error: close failed");
            }
        }
    }
}
