#pragma once

#include "ap/meta/log.h"
#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>
#include <sys/types.h>

namespace ap::mem {

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


} // namespace ap::mem