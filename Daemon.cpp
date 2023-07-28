#ifndef __linux__
#error "Daemon.cpp should not be compiled on non-Linux systems. Please update your meson build configuration accordingly."
#endif
#include "Forward.h"
#include "Scanner.h"
#include "Parser.h"
#include "Program.h"
#include "Interpreter.h"

#include <regex>
#include <filesystem>

extern "C" {
#include <unistd.h>
#include <fcntl.h>

#include <sys/prctl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/types.h>

#include <linux/audit.h>
#include <linux/filter.h>
#include <linux/seccomp.h>
#include <linux/unistd.h>

}

//NOTE: Consider having pledge for linux as a dependency: https://justine.lol/pledge/
//      the following nonsense is practically necessary to use Linux's suspiciously obtuse syscall security system,
//      but it'd be nice if it were all obscured into that dependency.

#define ArchField offsetof(struct seccomp_data, arch)

#define Allow(syscall) \
    BPF_JUMP(BPF_JMP+BPF_JEQ+BPF_K, SYS_##syscall, 0, 1), \
    BPF_STMT(BPF_RET+BPF_K, SECCOMP_RET_ALLOW)

struct sock_filter filter[] = {
    //validate arch
    BPF_STMT(BPF_LD+BPF_W+BPF_ABS, ArchField),
    BPF_JUMP( BPF_JMP+BPF_JEQ+BPF_K, AUDIT_ARCH_X86_64, 1, 0),
    BPF_STMT(BPF_RET+BPF_K, SECCOMP_RET_KILL),

    //load syscall
    BPF_STMT(BPF_LD+BPF_W+BPF_ABS, offsetof(struct seccomp_data, nr)),

    /* list of allowed syscalls */
    Allow(exit_group),  // exits a process
    Allow(brk),         // for malloc(), inside libc
    Allow(mmap),        // also for malloc()
    Allow(munmap),      // for free(), inside libc
    Allow(write),       // called by Joao himself
    Allow(read),        // called by Joao himself

    //and if we don't match above, die
    BPF_STMT(BPF_RET+BPF_K, SECCOMP_RET_KILL),
};
struct sock_fprog filterprog = {
    .len = sizeof(filter)/sizeof(filter[0]),
    .filter = filter
};


//Locks down the current running process to minimize hijinks.
static int ActivateSandbox() {
    // The specific recipe for this beautiful seccomp dish is from this blog post: https://eigenstate.org/notes/seccomp.html
    // (with extra garnishes and flavours from yours truly)
    int ret = 0;
    ret |= prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0); // Make sure we can't change our seccomp setup after it's been done.
    ret |= prctl(PR_SET_SECCOMP, SECCOMP_MODE_FILTER, &filterprog); // and then draw the rest of the fucking owl
    return ret;
}

namespace Daemon
{
std::vector<std::vector<Token>> cache;
const std::filesystem::path JOAO_INPUT_PIPE = "/tmp/JoaoPipeInput";
const std::filesystem::path JOAO_OUTPUT_PIPE = "/tmp/JoaoPipeOutput";
const std::filesystem::path JOAO_ERR_PIPE = "/tmp/JoaoPipeError";

using FileDescriptor = int;

//Executes a command-line command and returns the result.
std::string exec(const char* cmd) noexcept {
    std::array<char, 128> buffer;
    std::string result;
    FILE* pipePointer = popen(cmd, "r");
    if (!pipePointer) {
        return "";
    }
    while (fgets(buffer.data(), buffer.size(), pipePointer) != nullptr) {
        result += buffer.data();
    }
    pclose(pipePointer);
    return result;
}

std::string exec(const std::string& cmd) noexcept {
    return exec(cmd.c_str());
}

void DaemonError(int errorPipe, const std::string& what)
{
    write(errorPipe, what.c_str(),what.size());
}

void initialize()
{
    //To make a typical daemon, we need to estrange the daemon from the terminal that made us and its associated session,
    //thereby making our parent process be 'init' and turning us into a free-floating chunk of code in the background.
    pid_t my_pid = fork();

    if(my_pid < 0) UNLIKELY {
        std::cerr << "Joao daemon error: Failed to create child process for daemonification.\n";
        exit(EXIT_FAILURE);
    }
    if(my_pid > 0 ) {// If we are the parent
        std::cout << "Terminating parent process...\n";
        return; // Go die
    }

    //We are now the child process.
    if(setsid() < 0) UNLIKELY {
        std::cerr << "Joao daemon error: Failed to set session ID for daemonification.\n";
        exit(EXIT_FAILURE);
    }

    //Close standard i/o ports
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    //Open the pipe, if necessary
    if(!std::filesystem::is_fifo(JOAO_INPUT_PIPE)) // God bless C++17
        mkfifo(JOAO_INPUT_PIPE.c_str(), S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
    if(!std::filesystem::is_fifo(JOAO_OUTPUT_PIPE))
        mkfifo(JOAO_OUTPUT_PIPE.c_str(), S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
    if(!std::filesystem::is_fifo(JOAO_ERR_PIPE))
        mkfifo(JOAO_ERR_PIPE.c_str(), S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);    
    //FIXME: Here would be a great place to check to see if an old pipe exists
    //and it still has some data in it that was never read for some reason.
    //However, apparently (???) Linux doesn't have any way to peek a pipe, AFAIK, so.

    const FileDescriptor inputPipe = open(JOAO_INPUT_PIPE.c_str(),O_RDONLY);
    const FileDescriptor outputPipe = open(JOAO_OUTPUT_PIPE.c_str(),O_WRONLY);
    const FileDescriptor errorPipe = open(JOAO_ERR_PIPE.c_str(),O_WRONLY);
    if(inputPipe < 0 || outputPipe < 0 || errorPipe < 0) {
        return; // FIXME: Try to find a way to emit an error in this situation.
    }

    //WE ARE LOCKED DOWN PAST THIS POINT.
    ActivateSandbox();

    while(true) {
        std::array<char,256> readBuffer;
        std::string readString;
        // Doing some intentional i/o blocking here.
        ssize_t readBytes;
        while(readBytes = read(inputPipe,readBuffer.data(),readBuffer.size() > 0)) {
            readString.append(readBuffer.data(),readBytes);
        }
        std::stringstream scriptStream(readString);
        try {
            Scanner scn;
            scn.scan(scriptStream);
            Parser prs(scn);
            Program parsed = prs.parse();
            Interpreter interpreter(parsed,false);
            
            std::vector<Value> joao_args;
            Value jargs = interpreter.makeBaseTable(joao_args,{},nullptr);

            //Execute!
            Value v = interpreter.execute(parsed, jargs);
            std::string resultString = v.to_string();
            write(outputPipe,resultString.c_str(),resultString.size());
        } catch(error::scanner scannerError) {
            write(errorPipe, scannerError.what(), strlen(scannerError.what()));
        } catch(error::parser parserError) {
            write(errorPipe, parserError.what(), strlen(parserError.what()));
        }
    }
}

//Finds and halts the daemon, if there is one.
void stop()
{
    const std::string daemonList = exec("ps -eo 'tty,pid,comm'"); // Stack Overflow says this lists all daemons running.
    const std::regex daemonRegex = std::regex(R"(\?\s+(\d+) joao)"); // ps outputs in a semi-consistent pattern, we can regex agains it.
    std::smatch daemonRegexMatch;
    if(!std::regex_search(daemonList,daemonRegexMatch, daemonRegex) || daemonRegexMatch.size() < 2) {
        std::cout << "No daemon found to stop.\n";
        return;
    }
    std::cout << "Halting daemon...\n";
    // -2 is the CTRL+C interrupt signal. Gives the daemon some room to do cleanup before dying.
    // FIXME: Add a forceful mode for when Joao really needs to die right this second.
    std::string killCommand = "kill -2 " + daemonRegexMatch[1].str();
    std::cout << exec(killCommand);
}

}
