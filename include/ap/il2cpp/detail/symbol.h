#pragma once

#include "ptrace.h"
#include <array>
#include <cstdint>
#include <optional>
#include <sys/types.h>

namespace ap::il2cpp::detail {

struct symbol_caller {
    std::uintptr_t addr;

    template <typename... Args>
    inline std::optional<uint64_t> invoke(Args... args) noexcept {
        std::array<uint64_t, sizeof...(Args)> array{static_cast<uint64_t>(args)...};
        return g_ptrace_controller.call(addr, array);
    }

    static std::optional<symbol_caller>
    get_caller(const std::string &symbol) noexcept;

    template <typename Pid>
    inline static void init_caller(Pid pid, std::uintptr_t base) noexcept {
        g_pid = static_cast<pid_t>(pid);
        g_base_addr = base;

        init_symbol();
        g_ptrace_controller.set_pid(g_pid);
        g_ptrace_controller.attach();
    }

    inline static pid_t g_pid = 0;
    inline static std::uintptr_t g_base_addr = 0;
    inline static ptrace_controller g_ptrace_controller{};

  private:
    static void init_symbol() noexcept;
};

} // namespace ap::il2cpp::detail