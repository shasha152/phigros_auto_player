#include "ap/il2cpp/detail/symbol.h"
#include "ap/mem/map.h"
#include "ap/mem/mem.h"
#include "ap/meta/log.h"

#include <dlfcn.h>
#include <elfio/elfio.hpp>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <sys/types.h>

namespace ap::il2cpp::detail {
static std::unordered_map<std::string, symbol_caller> g_symbol;

template <typename Func>
void for_each_symbol(std::string_view path, const Func &func) noexcept {
    if (!std::filesystem::exists(path))
        return;

    ELFIO::elfio reader;
    reader.load(path.data());

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

                if (type == ELFIO::STT_FUNC)
                    func(name, value);
            }
        }
    }
}

std::string find_app_lib(std::string_view name) {
    mem::map64 map{symbol_caller::g_pid};
    map.parse_only(name);

    if (!map.get_entries().empty())
        return map.get_entries().at(0).parent_path;

    return "";
}

void symbol_caller::init_symbol(std::string_view lib) noexcept {
    auto path = find_app_lib(lib);

    for_each_symbol(path, [](auto name, auto value) {
        if (std::string_view(name).find("il2cpp") != std::string::npos)
            g_symbol.emplace(name, g_base_addr + value);
    });
}

std::optional<symbol_caller>
symbol_caller::get_caller(const std::string &symbol) noexcept {
    auto it = g_symbol.find(symbol);
    if (it == g_symbol.end()) {
        LOGW("symbol_caller::get_caller: %s not found", symbol.c_str());
        return std::nullopt;
    }

    return it->second;
}
std::uintptr_t get_libc_symbol(const char *symbol) noexcept {
    struct libc_base_init {
        std::uintptr_t addr;

        libc_base_init(pid_t pid) noexcept {
            mem::accessor reader{pid};

            mem::map64 map{pid};
            map.parse_only("libc.so");

            addr = map.get_module(reader)->start;

            LOGI("libc_base: %lx", addr);
        }
    };

    static libc_base_init base{symbol_caller::g_pid};
    static void *handle = dlopen("libc.so", RTLD_NOW);

    auto sym = dlsym(handle, symbol);

    Dl_info info{};
    if (!dladdr(sym, &info))
        return 0;

    auto offset = reinterpret_cast<uintptr_t>(sym) -
                  reinterpret_cast<uintptr_t>(info.dli_fbase);

    return base.addr + offset;
}
} // namespace ap::il2cpp::detail