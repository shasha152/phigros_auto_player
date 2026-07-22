#pragma once

#include <sys/types.h>

struct iovec;

namespace ap::mem::detail {

ssize_t vm_readv(pid_t pid, iovec *local, iovec *remote, int count) noexcept;
ssize_t vm_writev(pid_t pid, iovec *local, iovec *remote, int count) noexcept;

} // namespace ap::mem::detail