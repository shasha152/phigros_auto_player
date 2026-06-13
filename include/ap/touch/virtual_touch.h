#pragma once

#include "finger.h"
#include <memory>
#include <vector>

namespace ap::touch {
class virtual_touch {
    int uinput_fd;
    int event_fd;

    std::vector<finger> fingers;

  public:
    static std::unique_ptr<virtual_touch> create();

    ~virtual_touch();
};
} // namespace ap::touch