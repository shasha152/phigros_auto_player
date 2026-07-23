#pragma once

#include <sys/types.h>
#include <type_traits>
#include <unordered_map>

namespace ap::il2cpp::detail {

struct symbol_caller {
    std::uintptr_t addr;

    template <typename Ret, typename... Args>
    Ret invoke(Args... args) noexcept {
        if constexpr (std::is_same_v<Ret, void>) {
            return;
        }
        return;
    }

    static const std::unordered_map<std::string, symbol_caller> &
    get_all_caller() noexcept;

    template <typename Pid>
    static void init_caller(Pid pid, std::uintptr_t base) noexcept {
        g_pid = static_cast<pid_t>(pid);
        g_base_addr = base;
    }

    inline static pid_t g_pid = 0;
    inline static std::uintptr_t g_base_addr = 0;
};

} // namespace ap::il2cpp::detail