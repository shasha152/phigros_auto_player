#pragma once

#include "detail/pid.h"
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <optional>
#include <string>
#include <vector>

namespace ap::mem {

template <typename AddrType> struct map_entry {
    AddrType start{0};
    AddrType end{0};
    std::string perm{4};
    std::string parent_path;

    // explicit map_entry(AddrType start, AddrType end,
    //                    std::string parent_path) noexcept
    //     : start(start), end(end), parent_path(std::move(parent_path)) {}
};

template <typename AddrType> class basic_map : public detail::base_pid {
    std::vector<map_entry<AddrType>> entries;
    std::ifstream file;

    using object_type = std::vector<map_entry<AddrType>>;

  public:
    template <typename Pid>
    explicit basic_map(Pid pid)
        : detail::base_pid(pid),
          file("/proc/" + std::to_string(get_pid()) + "/maps") {}

    void parse_all() noexcept {
        std::string str;
        entries.reserve(256);
        char buffer[1024];

        while (std::getline(file, str)) {
            auto &data = entries.emplace_back();
            std::sscanf(str.c_str(), "%lx-%lx %s %*s %*s %*s %s", &data.start,
                        &data.end, data.perm.data(), buffer);

            data.parent_path = buffer;
        }
    }

    void parse_only(std::string_view so_str) noexcept {
        entries.reserve(8);
        std::string str;

        while (std::getline(file, str)) {

            AddrType start, end;
            char buffer[256] = {};
            char perm[5] = {};

            std::sscanf(str.c_str(), "%lx-%lx %s %*s %*s %*s %s", &start, &end,
                        perm, buffer);
            if (std::string_view(buffer).find(so_str) != std::string::npos)
                entries.emplace_back(
                    map_entry<AddrType>{start, end, perm, buffer});
        }
    }

    const object_type &get_entries() const noexcept { return entries; }

    template <typename Reader>
    std::optional<typename object_type::value_type>
    get_module(Reader &reader) const noexcept {
        const map_entry<AddrType> *elf_head = nullptr;

        for (auto &e : get_entries()) {
            int mag = 0;
            reader.read(e.start, mag);

            if (mag == 0x464C457F) {
                elf_head = &e;
                continue;
            }

            if (e.perm[2] == 'x' && elf_head)
                return *elf_head;
        }

        return {};
    }
};

using map32 = basic_map<std::uint32_t>;
using map64 = basic_map<std::uint64_t>;

} // namespace ap::mem