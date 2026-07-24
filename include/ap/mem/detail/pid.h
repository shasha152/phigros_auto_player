#pragma once

#include "ap/meta/log.h"
#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>
#include <sys/types.h>

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

template <typename Pid>
inline static constexpr bool is_convertible_pid_v =
    std::is_convertible_v<std::remove_cvref_t<Pid>, std::string_view> ||
    std::is_convertible_v<std::remove_cvref_t<Pid>, pid_t>;

class base_pid {
    void _init(pid_t pid) noexcept { this->pid = pid; }
    void _init(std::string_view name) noexcept {
        pid = find_pid_of_cmdline(name);
    }

  protected:
    pid_t pid;

  public:
    template <typename Pid>
        requires is_convertible_pid_v<Pid>
    explicit base_pid(Pid pid) noexcept {
        _init(pid);
    }
    base_pid() noexcept : pid() {}

    pid_t get_pid() const noexcept { return pid; }
    void set_pid(pid_t pid) noexcept { this->pid = pid; }
};

} // namespace detail

} // namespace ap::mem