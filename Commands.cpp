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
    return !s.empty() &&
           std::find_if(s.begin(),
                        s.end(), [](unsigned char c) { return !std::isdigit(c); }) == s.end();
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
std::shared_ptr<Command> SmallShell::CreateCommand(const char* cmd_line) {

    string cmd_s = _trim(string(cmd_line));
    string firstWord = cmd_s.substr(0, cmd_s.find_first_of(" \n"));

    for (int i=0; i<cmd_s.length(); i++){ // TODO better parsing? maybe splice with <, <<, |&, |?
        if (cmd_s[i] == '|'){
            if ((i+1 < cmd_s.length()) and (cmd_s[i+1] == '&')){ // TODO consider if we want to split between the ops here or later
                return std::shared_ptr<Command>(new PipeCommand(cmd_line));
            }
            else { return std::shared_ptr<Command>(new PipeCommand(cmd_line)); } // op = '|'
        }
        else if (cmd_s[i] == '>'){
            if ((i+1 < cmd_s.length()) and (cmd_s[i+1] == '>')){
                return std::shared_ptr<Command>(new RedirectionCommand(cmd_line));
            }
            else { return std::shared_ptr<Command>(new RedirectionCommand(cmd_line)); }
        }
    }

    if (firstWord.compare("pwd") == 0){
        return std::shared_ptr<Command>(new GetCurrDirCommand(cmd_s.c_str()));
    }
    else if (firstWord.compare("showpid") == 0){
        return std::shared_ptr<Command>(new ShowPidCommand(cmd_s.c_str()));
    }
    else if (firstWord.compare("chprompt")==0){
        return std::shared_ptr<Command>(new ChangePromptCommand(cmd_s.c_str(), &prompt_line));
    }
    else if (firstWord.compare("cd")==0){
        return std::shared_ptr<Command>(new ChangeDirCommand(cmd_s.c_str(), &last_working_directory));
    }
    else if (firstWord.compare("jobs")==0){
        return std::shared_ptr<Command>(new JobsCommand(cmd_s.c_str(),&jobs));
    }
    else if (firstWord.compare("kill")==0){
        return std::shared_ptr<Command>(new KillCommand(cmd_s.c_str(), &jobs));
    }
    else if (firstWord.compare("fg")==0){
        return std::shared_ptr<Command>(new ForegroundCommand(cmd_s.c_str(), &jobs));
    }
    else if (firstWord.compare("bg")==0){
        return std::shared_ptr<Command>( new BackgroundCommand(cmd_s.c_str(), &jobs));
    }
    else if (firstWord.compare("quit")==0){
        return std::shared_ptr<Command>(new QuitCommand(cmd_s.c_str(), &jobs));
    }
    else if (firstWord.compare("cat")==0){
        return std::shared_ptr<Command>(new CatCommand(cmd_s.c_str()));
    }
    //else if (firstWord.compare(""))
    else {
        return std::shared_ptr<Command>(new ExternalCommand(cmd_s.c_str()));
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
            perror("smash: fork failed");
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
//                running_cmd = p; // TODO  maybe do this as well??
                waitpid(jobs.getForeGroundJob()->getJobPid(),nullptr, WUNTRACED);
            }
            else {
                jobs.addJob(cmd);
            }
        }
    }
    else {
        //if (cmd_line[0]=="") { // TODO handle forking external commands
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
    string temp(cmd_line);
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
                                                         bash_cmd(cmd_line)
{
    is_bg_cmd = _isBackgroundCommand(cmd_line);
    if(is_bg_cmd) {
        _removeBackgroundSign(bash_cmd);
    }
    //TODO: this init will be overridden as soon as the wrapper function will fork!!
    //   cmd_pid = 0;


}



void ShowPidCommand::execute() {
    pid_t pid = getpid();
    cout << "smash job_pid is "<< pid << "\n";
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

void Command::setCmdPid(pid_t cmdPid) {
    cmd_pid = cmdPid;
}

pid_t Command::getCmdPid() const {
    return cmd_pid;
}

ostream &operator<<(ostream &os, const Command &command) {
    os << command.cmd_line;
    return os;
}


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

//shared_ptr<JobEntry> JobsList::getLastJobId(int* lastJobId){} // TODO last job that was done? or something else?

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
    // TODO does sending SIGCONT to a finished process could fail?
    kill(fg_job->getJobPid(), SIGCONT);
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
    // TODO does sending SIGCONT to a finished process could fail?
    MarkCont(job_id);
    kill(jobs[job_id]->getJobPid(), SIGCONT);
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
    if(not jobs->isExists(job_id)) {
        cerr << "smash error: kill: job-id " << job_id <<  " does not exist" << endl;
        return;
    }
    if(kill(jobs->getPIDByJobId(job_id),signal) == -1) {
        perror("smash error: kill failed");
    }

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
            cerr << "smash error: fg: " << job_id << " does not exist" << endl;
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
            cerr << "smash error: bg: " << job_id << " does not exist" << endl;
            return;
        }
    }
    else if(num_arg == 1) {
        if(job_id == -1) {
            // getLastJobId return -1 --> jobs list is empty
            cerr << "smash error: bg: jobs list is empty" << endl;
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

PipeCommand::PipeCommand(const char *cmd_line) : BuiltInCommand(cmd_line) {}

void PipeCommand::execute() {
    SmallShell& smash = SmallShell::getInstance();
    int fd [2];
    if (pipe(fd) == -1){
        //TODO Failure
    }

    pid_t p = fork();
    if (p == -1) {
        // fork failed
        perror("smash: fork failed");
    }
    else if (p == 0){
        close(fd[0]);

    }
    else {
        //father code
    }
}

RedirectionCommand::RedirectionCommand(const char *cmd_line) : BuiltInCommand(cmd_line) {}

void RedirectionCommand::execute() {
    SmallShell &smash = SmallShell::getInstance();
    string op = this->arguments[1]; // ">" or ">>". TODO: can this be someplace other than arguments[1]? if so, loop?

    int fd_num;


    pid_t p = fork(); //TODO Piazza said don't fork. tut3 page43 does fork though (and uses execv). what to do? I think it's possible without forking, but how?
    if (p == -1) {
        // fork failed
        perror("smash: fork failed");
    }
    else if (p == 0){
        close(1);
        if (op == ">") {
            fd_num = open(this->arguments[2], O_WRONLY | O_CREAT | O_TRUNC, 0666);
        }
        if (op == ">>") {
            fd_num = open(this->arguments[2], O_APPEND | O_WRONLY | O_CREAT, 0666);
        }
        char* arguments[] = {this->arguments[0], NULL};
        execv(arguments[0], arguments);
        //write(fd_num, buffer, size); TODO how?
    }
    else{ // father code
        close(0);
        wait(NULL);
    }

    //TODO this is not writing to the file properly. I assume we need to use write(...), but how to do this while executing the command?
    //std::shared_ptr<Command> cmd = smash.CreateCommand(this->arguments[0]);
    //cmd->execute();
    //cout << getpid() << endl;
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
