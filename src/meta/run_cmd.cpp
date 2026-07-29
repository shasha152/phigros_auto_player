#include "ap/meta/run_cmd.h"
#include <cstdio>
#include <cstring>
#include <stdio.h>
#include <string>
#include <string_view>

namespace ap::meta {
std::string run_cmd(std::string_view cmd) noexcept {
    std::string res;
    FILE *file = popen(cmd.data(), "r");

    if (!file)
        return res;

    char line[1024] = {0};
    while (fgets(line, sizeof(line), file)) {
        res.append(line);
        memset(line, 0, sizeof(line));
    }

    pclose(file);

    return res;
}
} // namespace ap::meta