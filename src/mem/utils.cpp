#include "ap/mem/detail/utils.h"
#include "ap/mem/detail/pid.h"
#include <sys/syscall.h>
#include <unistd.h>

#ifndef __NR_process_vm_readv
#define __NR_process_vm_readv 270
#endif

#ifndef __NR_process_vm_writev
#define __NR_process_vm_writev 271
#endif

namespace ap::mem {

self::operator pid_t() const noexcept { return getpid(); }

namespace detail {

ssize_t vm_readv(pid_t pid, iovec *local, iovec *remote, int count) noexcept {
    return syscall(__NR_process_vm_readv, pid, local, count, remote, count, 0);
}

ssize_t vm_writev(pid_t pid, iovec *local, iovec *remote, int count) noexcept {
    return syscall(__NR_process_vm_writev, pid, local, count, remote, count, 0);
}
} // namespace detail
} // namespace ap::mem