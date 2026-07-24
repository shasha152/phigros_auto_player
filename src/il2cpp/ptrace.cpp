#include "ap/il2cpp/detail/ptrace.h"
#include "ap/meta/log.h"

#include <asm-generic/signal.h>
#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <linux/elf.h>
#include <linux/ptrace.h>
#include <linux/wait.h>
#include <optional>
#include <signal.h>
#include <sys/ptrace.h>
#include <sys/uio.h>
#include <sys/user.h>
#include <sys/wait.h>

#define ERROR_LOG(tag, err)                                                    \
    if (!err)                                                                  \
    LOGE(tag ": %s", strerror(errno))

namespace ap::il2cpp::detail {
bool ptrace_attach(pid_t pid) noexcept {
    bool res = ptrace(PTRACE_ATTACH, pid, nullptr, nullptr) == 0;
    ERROR_LOG("ptrace_attach", res);

    waitpid(pid, nullptr, __WALL);

    return res;
}

bool ptrace_detach(pid_t pid) noexcept {
    kill(pid, SIGSTOP);
    waitpid(pid, nullptr, __WALL);

    bool res = ptrace(PTRACE_DETACH, pid, nullptr, nullptr) == 0;
    ERROR_LOG("ptrace_detach", res);

    return res;
}

bool ptrace_getregs(pid_t pid, user_regs_struct *regs) noexcept {
    struct iovec iov = {regs, sizeof(struct user_regs_struct)};
    bool res = ptrace(PTRACE_GETREGSET, pid, (void *)NT_PRSTATUS, &iov) == 0;

    ERROR_LOG("ptrace_getregs", res);
    return res;
}

bool ptrace_setregs(pid_t pid, user_regs_struct *regs) noexcept {
    struct iovec iov = {regs, sizeof(struct user_regs_struct)};
    bool res = ptrace(PTRACE_SETREGSET, pid, (void *)NT_PRSTATUS, &iov) == 0;

    ERROR_LOG("ptrace_setregs", res);
    return res;
}

bool ptrace_continue(pid_t pid) noexcept {
    bool res = ptrace(PTRACE_CONT, pid, nullptr, nullptr) == 0;
    ERROR_LOG("ptrace_continue", res);

    return res;
}

std::optional<uint64_t> ptrace_call(pid_t pid, uintptr_t func,
                                    std::span<uint64_t> args) noexcept {
    user_regs_struct regs, backup;

    kill(pid, SIGSTOP);
    waitpid(pid, nullptr, __WALL);

    if (!ptrace_getregs(pid, &regs))
        return {};
    backup = regs;

    for (size_t i = 0; i < args.size() && i < 8; i++)
        regs.regs[i] = args[i];
    regs.regs[30] = 0;

    regs.pc = func;
    // regs.regs[30] = trap_addr;

    ptrace_setregs(pid, &regs);
    ptrace_continue(pid);

    waitpid(pid, nullptr, __WALL);

    if (!ptrace_getregs(pid, &regs))
        return {};

    uint64_t ret = regs.regs[0];

    ptrace_setregs(pid, &backup);
    ptrace_continue(pid);

    return ret;
}
} // namespace ap::il2cpp::detail