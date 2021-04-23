#ifndef SMASH_COMMAND_H_
#define SMASH_COMMAND_H_
#include <vector>

#define DO_SYS( syscall ) do { \
    if((syscall) == -1 ) { \
    perror( #syscall ); \
    exit(1); \
    } \
    } while( 0 ) \

#define COMMAND_ARGS_MAX_LENGTH (200)
#define COMMAND_MAX_ARGS (30)
#define PATH_ARG 1
const std::string WHITESPACE = " \n\r\t\f\v";
int _parseCommandLine(const char* cmd_line, char** args);

class Command {
// TODO: Add your data members
protected:
    int num_arg;
    char* arguments[COMMAND_MAX_ARGS];
public:
    Command(const char* cmd_line= "");
    virtual ~Command();
    virtual void execute() = 0;
    //virtual void prepare();
    //virtual void cleanup();
    // TODO: Add your extra methods if needed
};

class BuiltInCommand : public Command {
private:
protected:
//    int num_arg;
//    char* arguments[COMMAND_MAX_ARGS];
public:
    BuiltInCommand(const char* cmd_line= "");
    virtual ~BuiltInCommand();
};

class ExternalCommand : public Command {
    std::string bash_cmd;
public:
    ExternalCommand(const char* cmd_line);
    virtual ~ExternalCommand();
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

// TODO: add chprompt
class ChangePromptCommand : public BuiltInCommand {
    std::string* smash_prompt;
public:
    ChangePromptCommand(const char *cmd_line, std::string *smash_prompt); // TODO more arguments maybe
    virtual ~ChangePromptCommand() {}
    void execute() override;
};

// TODO cd
class ChangeDirCommand : public BuiltInCommand {
private:
// TODO: Add your data members public:
public:
    std::string* oldpwd;
    ChangeDirCommand(const char *cmd_line, std::string *oldpwd);
    virtual ~ChangeDirCommand() = default;
    void execute() override;
};

// TODO pwd
class GetCurrDirCommand : public BuiltInCommand {
public:
    GetCurrDirCommand(const char* cmd_line ) {}
    virtual ~GetCurrDirCommand() {}
    void execute() override;
};

// TODO showpid
class ShowPidCommand : public BuiltInCommand {
public:
    ShowPidCommand(const char* cmd_line) {}
    virtual ~ShowPidCommand() {}
    void execute() override;
};


class JobsList;
class QuitCommand : public BuiltInCommand {
// TODO: Add your data members public:
    QuitCommand(const char* cmd_line, JobsList* jobs);
    virtual ~QuitCommand() {}
    void execute() override;
};




class JobsList {
public:
    class JobEntry {
        // TODO: Add your data members
    };
    // TODO: Add your data members
public:
    JobsList();
    ~JobsList();
    void addJob(Command* cmd, bool isStopped = false);
    void printJobsList();
    void killAllJobs();
    void removeFinishedJobs();
    JobEntry * getJobById(int jobId);
    void removeJobById(int jobId);
    JobEntry * getLastJob(int* lastJobId);
    JobEntry *getLastStoppedJob(int *jobId);
    // TODO: Add extra methods or modify existing ones as needed
};

class JobsCommand : public BuiltInCommand {
    // TODO: Add your data members
public:
    JobsCommand(const char* cmd_line, JobsList* jobs);
    virtual ~JobsCommand() {}
    void execute() override;
};

class KillCommand : public BuiltInCommand {
    // TODO: Add your data members
public:
    KillCommand(const char* cmd_line, JobsList* jobs);
    virtual ~KillCommand() {}
    void execute() override;
};

class ForegroundCommand : public BuiltInCommand {
    // TODO: Add your data members
public:
    ForegroundCommand(const char* cmd_line, JobsList* jobs);
    virtual ~ForegroundCommand() {}
    void execute() override;
};

class BackgroundCommand : public BuiltInCommand {
    // TODO: Add your data members
public:
    BackgroundCommand(const char* cmd_line, JobsList* jobs);
    virtual ~BackgroundCommand() {}
    void execute() override;
};

class CatCommand : public BuiltInCommand {
public:
    CatCommand(const char* cmd_line);
    virtual ~CatCommand() {}
    void execute() override;
};


class SmallShell {
private:
    std::string prompt_line;
    std::string last_working_directory;
    pid_t running_cmd;
    SmallShell();
public:
    const std::string &getPromptLine() const;
    pid_t getRunningCmd() const;
    Command *CreateCommand(const char* cmd_line);
    SmallShell(SmallShell const&)      = delete; // disable copy ctor
    void operator=(SmallShell const&)  = delete; // disable = operator
    static SmallShell& getInstance() // make SmallShell singleton
    {
        static SmallShell instance; // Guaranteed to be destroyed.
        // Instantiated on first use.
        return instance;
    }
    ~SmallShell();
    void executeCommand(const char* cmd_line);
    // TODO: add extra methods as needed
};

#endif //SMASH_COMMAND_H_
