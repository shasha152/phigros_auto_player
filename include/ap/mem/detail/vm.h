#pragma once

#include "ap/mem/detail/utils.h"
#include "pid.h"
#include <cstdint>
#include <linux/uio.h>
#include <sys/types.h>

namespace ap::mem::detail {
class vm_rdwr : public base_pid {
  public:
    using return_type = int;

    template <typename Pid>
        requires is_convertible_pid_v<Pid>
    explicit vm_rdwr(Pid pid) noexcept : base_pid(pid) {}

    int read(std::uintptr_t addr, void *buffer, std::size_t len) noexcept {
        iovec local{.iov_base = buffer, .iov_len = len};
        iovec remote{.iov_base = reinterpret_cast<void *>(addr),
                     .iov_len = len};

        return read(&local, &remote, 1);
    }

    int write(std::uintptr_t addr, void *buffer, std::size_t len) noexcept {
        iovec local{.iov_base = buffer, .iov_len = len};
        iovec remote{.iov_base = reinterpret_cast<void *>(addr),
                     .iov_len = len};

        return write(&local, &remote, 1);
    }

    int read(iovec *local, iovec *remote, int count) noexcept {
        return vm_readv(get_pid(), local, remote, count);
    }

    int write(iovec *local, iovec *remote, int count) noexcept {
        return vm_writev(get_pid(), local, remote, count);
    }
};
} // namespace ap::mem::detail