#include "ap/touch/virtual_touch.h"

#include "ap/meta/log.h"

#include <fcntl.h>
#include <unistd.h>

namespace ap::touch {

std::unique_ptr<virtual_touch> virtual_touch::create() {
    auto touch = std::make_unique<virtual_touch>();
    touch->uinput_fd = open("/dev/uinput", O_RDWR);

    if (touch->uinput_fd) {
        LOGW("virtual_touch::create: /dev/uinput 打开失败");
        return nullptr;
    }

    return touch;
}

virtual_touch::~virtual_touch() {
    close(uinput_fd);
    close(event_fd);
}
} // namespace ap::touch