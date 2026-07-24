#include "ap/il2cpp/detail/symbol.h"
#include "ap/meta/log.h"

#include <elfio/elfio.hpp>
#include <filesystem>
#include <fstream>
#include <optional>
#include <string>
#include <string_view>
#include <sys/types.h>

namespace ap::il2cpp::detail {
static std::unordered_map<std::string, symbol_caller> g_symbol;

std::string read_package_name(pid_t pid) noexcept {
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

void symbol_caller::init_symbol() noexcept {
    auto name = read_package_name(symbol_caller::g_pid);
    auto path = find_app_lib(name) + "/lib/arm64/libil2cpp.so";

    LOGI("init_il2_symbol: package=%s lib path=%s", name.c_str(), path.c_str());

    if (std::filesystem::exists(path)) {
        ELFIO::elfio reader;
        reader.load(path);

        for (const auto &sec : reader.sections) {
            if (sec->get_type() == ELFIO::SHT_SYMTAB ||
                sec->get_type() == ELFIO::SHT_DYNSYM) {
                ELFIO::symbol_section_accessor accessor(reader, sec.get());

                for (unsigned int i = 0; i < accessor.get_symbols_num(); ++i) {
                    std::string name;
                    ELFIO::Elf64_Addr value;
                    ELFIO::Elf_Xword size;
                    unsigned char bind;
                    unsigned char type;
                    ELFIO::Elf_Half section_index;
                    unsigned char other;

                    accessor.get_symbol(i, name, value, size, bind, type,
                                        section_index, other);

                    if (type == ELFIO::STT_FUNC &&
                        name.find("il2cpp") != std::string::npos)
                        g_symbol.emplace(name,
                                         symbol_caller::g_base_addr + value);
                }
            }
        }
    }
}

std::optional<symbol_caller>
symbol_caller::get_caller(const std::string &symbol) noexcept {
    auto it = g_symbol.find(symbol);
    if (it == g_symbol.end())
        return std::nullopt;

    return it->second;
}

} // namespace ap::il2cpp::detail