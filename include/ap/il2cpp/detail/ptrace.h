#pragma once

#include "ap/mem/detail/pid.h"
#include <cstdint>
#include <optional>
#include <span>
#include <sys/types.h>
#include <variant>

namespace ap::il2cpp::detail {

std::optional<uint64_t> ptrace_call(pid_t pid, std::uintptr_t addr,
                                    std::span<std::uint64_t> args) noexcept;

bool ptrace_attach(pid_t pid) noexcept;
bool ptrace_detach(pid_t pid) noexcept;
bool ptrace_continue(pid_t pid) noexcept;

class ptrace_controller : public mem::detail::base_pid {
    bool is_attach = false;

  public:
    template <typename Pid>
    explicit ptrace_controller(Pid pid, std::monostate) noexcept
        : mem::detail::base_pid(pid) {
        attach();
    }

    template <typename Pid>
    explicit ptrace_controller(Pid pid) noexcept : mem::detail::base_pid(pid) {}

    explicit ptrace_controller(std::monostate) noexcept
        : mem::detail::base_pid() {
        attach();
    }
    ptrace_controller() = default;

    bool attach() {
        if (is_attach)
            return true;

        is_attach = ptrace_attach(pid);
        ptrace_continue(pid);
        return is_attach;
    }

    std::optional<uint64_t> call(std::uintptr_t addr,
                                 std::span<std::uint64_t> args) {
        if (!is_attach)
            return std::nullopt;

        return ptrace_call(pid, addr, args);
    }

    ~ptrace_controller() { ptrace_detach(pid); }
};

} // namespace ap::il2cpp::detail