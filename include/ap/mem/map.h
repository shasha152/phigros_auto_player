#pragma once

#include <cstdint>
#include <cstdio>
#include <fstream>
#include <string>
#include <string_view>
#include <sys/types.h>
#include <vector>

namespace ap::mem {

template <typename AddrType> struct map_enrty {
    AddrType start;
    AddrType end;

    explicit map_enrty(AddrType start, AddrType end) noexcept
        : start(start), end(end) {}
};

template <typename AddrType> class basic_map {
    std::vector<map_enrty<AddrType>> entries;
    std::ifstream file;

    using object_type = std::vector<map_enrty<AddrType>>;

  public:
    explicit basic_map(pid_t pid)
        : file("/proc/" + std::to_string(pid) + "/maps") {}
    void parse_all() noexcept {
        std::string str;
        entries.reserve(256);

        while (std::getline(file, str)) {
            auto &data = entries.emplace_back();
            std::sscanf(str.c_str(), "%lx-%lx", &data.start, &data.end);
        }
    }

    void parse_only(std::string_view so_str) noexcept {
        entries.reserve(8);
        std::string str;

        while (std::getline(file, str)) {
            AddrType start, end;
            char buffer[256] = {};

            std::sscanf(str.c_str(), "%lx-%lx %*s %*s %*s %*s %s", &start, &end,
                        buffer);
            if (std::string_view(buffer).find(so_str) != std::string::npos) {
                entries.emplace_back(start, end);
            }
        }
    }

    const object_type &get_entries() const noexcept { return entries; }
};

using map32 = basic_map<std::uint32_t>;
using map64 = basic_map<std::uint64_t>;

} // namespace ap::mem