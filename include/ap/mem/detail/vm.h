#pragma once

#include "pid.h"
#include <cstdint>
#include <string_view>
#include <sys/types.h>
#include <type_traits>

namespace ap::mem::detail {
class vm_rdwr : public base_pid {
  public:
    using message_type = int;

    template <
        typename Pid,
        std::enable_if_t<
            std::is_convertible_v<std::remove_cvref_t<Pid>, std::string_view> ||
                std::is_convertible_v<std::remove_cvref_t<Pid>, pid_t>,
            int> = 0>
    explicit vm_rdwr(Pid pid) noexcept : base_pid(pid) {}

    int read(std::uintptr_t addr, const void *buffer,
             std::size_t count) noexcept {}
};
} // namespace ap::mem::detail