#pragma once

#include <unordered_map>

namespace ap::il2cpp::detail {

struct symbol_caller {
    std::uintptr_t addr;

    template <typename Ret, typename... Args>
    Ret invoke(Args... args) noexcept {
        return;
    }
};

static std::unordered_map<std::string, symbol_caller> g_symbol;

} // namespace ap::il2cpp::detail