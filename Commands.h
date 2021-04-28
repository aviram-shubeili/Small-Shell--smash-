#ifndef SMASH_COMMAND_H_
#define SMASH_COMMAND_H_
#include <vector>
#include <time.h>
#include <memory>
#include <ostream>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define DO_SYS( syscall ) do { \
    if((syscall) == -1 ) { \
    perror( #syscall ); \
    exit(1); \
    } \
    } while( 0 ) \

#define COMMAND_ARGS_MAX_LENGTH (200)
#define COMMAND_MAX_ARGS (30)
#define PATH_ARG 1
#define MAX_JOBS (101)
#define KILL_CMD_ARG_NUM 3
#define NO_RUNNING_CMD 0
#define READING_ITERATION 512
enum SmashOperation {
    QUIT = 0,
    CONTINUE
};
const std::string WHITESPACE = " \n\r\t\f\v";
int _parseCommandLine(const char* cmd_line, char** args);

class Command {
// TODO: Add your data members
    std::string cmd_line;
public:
    friend std::ostream &operator<<(std::ostream &os, const Command &command);

private:
    pid_t cmd_pid;
public:
    explicit Command(const char* cmd_line= "") : cmd_line(cmd_line),
                                                 cmd_pid(0) {}
    virtual ~Command() {}
    virtual void execute() = 0;
    //virtual void prepare();
    //virtual void cleanup();
    void setCmdPid(pid_t cmdPid);
    pid_t getCmdPid() const;
    // TODO: Add your extra methods if needed
};

class BuiltInCommand : public Command {
private:
protected:
    int num_arg;
    char* arguments[COMMAND_MAX_ARGS];
public:
    BuiltInCommand(const char* cmd_line);
    virtual ~BuiltInCommand();
};

class ExternalCommand : public Command {
    std::string bash_cmd;
public:
    bool is_bg_cmd;
    explicit ExternalCommand(const char* cmd_line);
    ~ExternalCommand() override = default;
    void execute() override;
};

class PipeCommand : public Command {
    // TODO: Add your data members
public:
    PipeCommand(const char* cmd_line);
    virtual ~PipeCommand() {}
    void execute() override;
};

class RedirectionCommand : public Command {
    // TODO: Add your data members
public:
    explicit RedirectionCommand(const char* cmd_line);
    virtual ~RedirectionCommand() {}
    void execute() override;
    //void prepare() override;
    //void cleanup() override;
};


class ChangePromptCommand : public BuiltInCommand {
    std::string* smash_prompt;
public:
    ChangePromptCommand(const char *cmd_line, std::string *smash_prompt); // TODO more arguments maybe
    virtual ~ChangePromptCommand() {}
    void execute() override;
};


class ChangeDirCommand : public BuiltInCommand {
private:

public:
    std::string* oldpwd;
    ChangeDirCommand(const char *cmd_line, std::string *oldpwd);
    virtual ~ChangeDirCommand() = default;
    void execute() override;
};


class GetCurrDirCommand : public BuiltInCommand {
public:
    GetCurrDirCommand(const char* cmd_line ) : BuiltInCommand(cmd_line) {}
    virtual ~GetCurrDirCommand() {}
    void execute() override;
};


class ShowPidCommand : public BuiltInCommand {
public:
    ShowPidCommand(const char* cmd_line) : BuiltInCommand(cmd_line) {}
    virtual ~ShowPidCommand() {}
    void execute() override;
};


class JobsList;
class QuitCommand : public BuiltInCommand {
    JobsList* jobs;
// TODO: Add your data members public:
public:
    QuitCommand(const char* cmd_line, JobsList* jobs);
    virtual ~QuitCommand() {}
    void execute() override;
};




class JobsList {
    class JobEntry {
    private:
        std::shared_ptr<Command> cmd;
        time_t time_executed;
        bool is_stopped;
        pid_t job_pid;
        int job_id;
    public:
        JobEntry(std::shared_ptr<Command> &cmd, pid_t p, bool is_stopped = false, int id= 0);
        ~JobEntry() = default;
        std::shared_ptr<Command> getCommand() { return cmd; }
        time_t getTime() const { return time_executed; }
        void resetTime();
        bool isStopped() const { return is_stopped; }
        pid_t getJobPid() const { return  job_pid; }
        void setIsStopped(bool isStopped);
    };
    // TODO: Add your data members
private:
    std::vector<std::shared_ptr<JobEntry>> jobs;
    std::shared_ptr<JobEntry> fg_job;
    int getMinFreeID();
//    std::vector<int> to_clear;
public:
    JobsList();
    ~JobsList() = default;
    void addJob(std::shared_ptr<Command> cmd, bool isStopped = false);
    void printJobsList();
    void killAllJobs();
    void removeFinishedJobs();
    std::shared_ptr<JobEntry> getJobById(int jobId);
    pid_t getPIDByJobId(int jobId);
    // TODO: getByPID?
    void removeJobById(int jobId);
    int getLastJobId(int* lastJobId);
    std::shared_ptr<JobEntry> getLastStoppedJob(int *jobId);
    // TODO: Add extra methods or modify existing ones as needed

    void setForeGroundJob(std::shared_ptr<Command> fg_cmd);
    const std::shared_ptr<JobEntry> &getForeGroundJob() const;
    void moveBGToFG(int job_id); // todo: implement this!
    void StopFG();
    void MarkStopped(int job_id);
    void ContinueJob(int job_id);
    void MarkCont(int job_id);
    bool isExists(int job_id);
    bool isStopped(int job_id);
    friend class SmallShell;

};

class JobsCommand : public BuiltInCommand {
JobsList* jobs;
public:
    JobsCommand(const char *cmd_line, JobsList* jobs);
    virtual ~JobsCommand() {}
    void execute() override;
};

class KillCommand : public BuiltInCommand {
    JobsList* jobs;
    int signal;
    int job_id;
    bool getArguments();
    // TODO: Add your data members
public:
    KillCommand(const char* cmd_line, JobsList* jobs);
    virtual ~KillCommand() {}
    void execute() override;
};

class ForegroundCommand : public BuiltInCommand {
    JobsList* jobs;
    int job_id;
    // TODO: Add your data members
    bool getArguments();
public:
    ForegroundCommand(const char* cmd_line, JobsList* jobs);
    virtual ~ForegroundCommand() {}
    void execute() override;
};

class BackgroundCommand : public BuiltInCommand {
    JobsList* jobs;
    int job_id;
    bool getArguments();
public:
    BackgroundCommand(const char* cmd_line, JobsList* jobs);
    virtual ~BackgroundCommand() {}
    void execute() override;
};

class CatCommand : public BuiltInCommand {
public:
    explicit CatCommand(const char* cmd_line);
    virtual ~CatCommand() {}
    void execute() override;
};


class SmallShell {
private:
    std::string prompt_line;
    std::string last_working_directory;
//    pid_t running_cmd;
    SmallShell();
public:
    JobsList jobs;
    const std::string &getPromptLine() const;
    pid_t getRunningCmd() const;
    std::shared_ptr<Command> CreateCommand(const char* cmd_line);
    SmallShell(SmallShell const&)      = delete; // disable copy ctor
    void operator=(SmallShell const&)  = delete; // disable = operator
    static SmallShell& getInstance() // make SmallShell singleton
    {
        static SmallShell instance; // Guaranteed to be destroyed.
        // Instantiated on first use.
        return instance;
    }
    ~SmallShell();
    SmashOperation executeCommand(const char* cmd_line);


    // TODO: add extra methods as needed
};

#endif //SMASH_COMMAND_H_
