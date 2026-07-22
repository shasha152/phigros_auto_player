#pragma once

#include "ap/meta/log.h"
#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>
#include <sys/types.h>
#include <unistd.h>

namespace ap::mem {

struct self {
    operator pid_t() const noexcept;
};

namespace detail {
inline pid_t find_pid_of_cmdline(std::string_view name) noexcept {
    std::string str;

    for (auto dir : std::filesystem::directory_iterator("/proc")) {
        auto path = dir.path().string() + "/cmdline";

        if (std::filesystem::exists(path)) {
            std::ifstream file(path);
            std::getline(file, str, '\0');

            if (str == name)
                return std::stoi(dir.path().filename());
        }
    }

    LOGW("find_pid_of_cmdline: 获取%sPID失败", name.data());
    return 0;
}

class base_pid {
  protected:
    pid_t pid;

  public:
    explicit base_pid(pid_t pid) noexcept : pid(pid) {}
    explicit base_pid(std::string_view name) noexcept
        : pid(find_pid_of_cmdline(name)) {}

    pid_t get_pid() const noexcept { return pid; }
};

} // namespace detail

} // namespace ap::mem