#ifndef __linux__
#error "Daemon.cpp should not be compiled on non-Linux systems. Please update your meson build configuration accordingly."
#endif
#include "Forward.h"
#include "Scanner.h"
#include "Parser.h"
#include "Program.h"
#include "Interpreter.h"

#include <memory>
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

#define ArchField offsetof(struct seccomp_data, arch)

#define Allow(syscall) \
    BPF_JUMP(BPF_JMP+BPF_JEQ+BPF_K, SYS_##syscall, 0, 1), \
    BPF_STMT(BPF_RET+BPF_K, SECCOMP_RET_ALLOW)

struct sock_filter filter[] = {
    /* validate arch */
    BPF_STMT(BPF_LD+BPF_W+BPF_ABS, ArchField),
    BPF_JUMP( BPF_JMP+BPF_JEQ+BPF_K, AUDIT_ARCH_X86_64, 1, 0),
    BPF_STMT(BPF_RET+BPF_K, SECCOMP_RET_KILL),

    /* load syscall */
    BPF_STMT(BPF_LD+BPF_W+BPF_ABS, offsetof(struct seccomp_data, nr)),

    /* list of allowed syscalls */
    Allow(exit_group),  /* exits a process */
    Allow(brk),         /* for malloc(), inside libc */
    Allow(mmap),        /* also for malloc() */
    Allow(munmap),      /* for free(), inside libc */
    Allow(write),       /* called by Joao himself */
    Allow(read),        /* called by Joao himself */

    /* and if we don't match above, die */
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
    ret |= prctl(PR_SET_SECCOMP, SECCOMP_MODE_FILTER, &filterprog);
    return ret;
}

namespace Daemon
{
std::vector<std::vector<Token>> cache;
const std::filesystem::path JOAO_INPUT_PIPE = "/tmp/JoaoPipeInput";
const std::filesystem::path JOAO_OUTPUT_PIPE = "/tmp/JoaoPipeOutput";

using FileDescriptor = int;

std::string exec(const char* cmd) noexcept {
    std::array<char, 128> buffer;
    std::string result;
    std::unique_ptr<FILE, decltype(&pclose)> pipePointer (popen(cmd, "r"), pclose);
    if (!pipePointer) {
        return "";
    }
    while (fgets(buffer.data(), buffer.size(), pipePointer.get()) != nullptr) {
        result += buffer.data();
    }
    return result;
}

[[noreturn]] void derror(const std::string& what)
{
    std::cout << "Joao daemon error: " << what << '\n';
    exit(EXIT_FAILURE);
}

void initialize()
{
    //To make a typical daemon, we need to estrange the daemon from the terminal that made us and its associated session,
    //thereby making our parent process be 'init' and turning us into a free-floating chunk of code in the background.
    pid_t my_pid = fork();

    if(my_pid < 0) UNLIKELY
        derror("Failed to create child process for daemonification.");
    
    if(my_pid > 0 )// If we are the parent
        return; // Go die

    //We are now the child process.
    if(setsid() < 0) UNLIKELY
        derror("Failed to set session ID for daemonification.");

    //Close standard i/o ports
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    //Open the pipe, if necessary
    if(!std::filesystem::is_fifo(JOAO_INPUT_PIPE)) // God bless C++17
        mkfifo(JOAO_INPUT_PIPE.c_str(), S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
    if(!std::filesystem::is_fifo(JOAO_OUTPUT_PIPE))
        mkfifo(JOAO_OUTPUT_PIPE.c_str(), S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
    //FIXME: Here would be a great place to check to see if an old pipe exists
    //and it still has some data in it that was never read for some reason.
    //However, apparently (???) Linux doesn't have any way to peek a pipe, AFAIK, so.

    FileDescriptor inputPipe = open(JOAO_INPUT_PIPE.c_str(),O_RDONLY);
    FileDescriptor outputPipe = open(JOAO_OUTPUT_PIPE.c_str(),O_WRONLY);
    if(inputPipe < 0 || outputPipe < 0) {
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
        Scanner scn;
        scn.scan(scriptStream);
        if(scn.is_malformed)
            continue; // TODO
        Parser prs(scn);
        Program parsed = prs.parse();
	    Interpreter interpreter(parsed,false);
        
        std::vector<Value> joao_args;
        Value jargs = interpreter.makeBaseTable(joao_args,{},nullptr);

        //Execute!
        Value v = interpreter.execute(parsed, jargs);
    }
}

void stop()
{

}

}