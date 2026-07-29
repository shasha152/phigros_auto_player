#pragma once

#include "ap/mem/detail/utils.h"
#include "pid.h"
#include <cstdint>
#include <linux/uio.h>
#include <sys/types.h>

namespace ap::mem::detail {
class vm_rdwr : public base_pid {
  public:
    using return_type = bool;

    template <typename Pid>
    explicit vm_rdwr(Pid pid) noexcept : base_pid(pid) {}

    bool read(std::uintptr_t addr, void *buffer, std::size_t len) noexcept {
        iovec local{.iov_base = buffer, .iov_len = len};
        iovec remote{.iov_base = reinterpret_cast<void *>(addr),
                     .iov_len = len};

        return read(&local, &remote, 1);
    }

    bool write(std::uintptr_t addr, void *buffer, std::size_t len) noexcept {
        iovec local{.iov_base = buffer, .iov_len = len};
        iovec remote{.iov_base = reinterpret_cast<void *>(addr),
                     .iov_len = len};

        return write(&local, &remote, 1);
    }

    bool read(iovec *local, iovec *remote, int count) noexcept {
        return vm_readv(get_pid(), local, remote, count) != -1;
    }

    bool write(iovec *local, iovec *remote, int count) noexcept {
        return vm_writev(get_pid(), local, remote, count) != -1;
    }
};
} // namespace ap::mem::detail