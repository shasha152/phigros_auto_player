#include "ap/il2cpp/detail/symbol.h"
#include "ap/meta/log.h"

#include <elfio/elfio.hpp>
#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>
#include <sys/types.h>

namespace ap::il2cpp::detail {
static std::unordered_map<std::string, symbol_caller> g_symbol;

std::string read_pack_name(pid_t pid) noexcept {
    std::string path = "/proc/" + std::to_string(pid) + "/cmdline";
    std::ifstream file(path);

    std::getline(file, path, '\0');

    return path;
}

std::string find_app_lib(std::string_view name) {
    for (auto entry : std::filesystem::directory_iterator("/data/app/")) {
        if (entry.is_directory()) {
            for (auto entry2 :
                 std::filesystem::directory_iterator(entry.path())) {
                if (entry2.path().filename().string().find(name) !=
                    std::string::npos)
                    return entry2.path().string();
            }
        }
    }

    return "";
}

void init_il2_symbol() noexcept {
    auto name = read_pack_name(symbol_caller::g_pid);
    auto path = find_app_lib(name) + "/lib/arm64/libil2cpp.so";

    LOGI("init_il2_symbol: package=%s lib path=%s", name.c_str(), path.c_str());

    if (std::filesystem::exists(path)) {
        ELFIO::elfio io;
        io.load(path);

        for (auto e : io.sections) {
            LOGI("symbol: %s=%lx", e->get_name().c_str(), e->get_address());
            g_symbol.emplace(e->get_name(),
                             symbol_caller{.addr = symbol_caller::g_base_addr +
                                                   e->get_address()});
        }
    }
}

const std::unordered_map<std::string, symbol_caller> &
symbol_caller::get_all_caller() noexcept {
    if (g_symbol.empty()) {
        init_il2_symbol();
    }
    return g_symbol;
}

} // namespace ap::il2cpp::detail