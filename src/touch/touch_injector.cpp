#include "ap/touch/detail/touch_injector.h"

#include "ap/meta/log.h"

#include <array>
#include <bits/ioctl.h>
#include <chrono>
#include <cstring>
#include <fcntl.h>
#include <filesystem>
#include <mutex>
#include <thread>
#include <unistd.h>

#include <linux/input.h>
#include <linux/uinput.h>
#include <vector>

namespace ap::touch::detail {

std::unique_ptr<touch_injector>
touch_injector::create(int max_forward_slot) noexcept {
    auto touch = std::make_unique<touch_injector>();
    touch->max_forward_slot = max_forward_slot;

    touch->uinput_fd = open("/dev/uinput", O_RDWR);
    if (touch->uinput_fd == -1) {
        LOGW("touch_injector::create: /dev/uinput 打开失败");
        return nullptr;
    }

    touch->event_fd = touch->scan_touch_device();
    if (touch->event_fd == -1) {
        LOGW("touch_injector::create: 查找触摸设备失败");
        return nullptr;
    }

    touch->init_device();
    touch->event_cache.reserve(32);

    return touch;
}

touch_injector::~touch_injector() {
    cleanup_finger();
    ioctl(event_fd, EVIOCGRAB, 0);

    stop_forward_worker();
    close(uinput_fd);
    close(event_fd);
}

int touch_injector::scan_touch_device() const {
    std::array<unsigned long, 64> abs_bits;

    for (auto entry : std::filesystem::directory_iterator("/dev/input")) {
        int fd = open(entry.path().c_str(), O_RDWR);
        if (ioctl(fd, EVIOCGBIT(EV_ABS, sizeof(abs_bits)), abs_bits.data()) <
            0) {
            close(fd);
            continue;
        }

        if (check_key_bit(ABS_MT_SLOT, abs_bits.data()) &&
            check_key_bit(ABS_MT_POSITION_X, abs_bits.data())) {
            LOGI("touch_injector::scan_touch_device: touch event path=%s",
                 entry.path().c_str());
            return fd;
        }

        close(fd);
    }
    return -1;
}

bool touch_injector::check_key_bit(unsigned int bit,
                                   const unsigned long *arr) const noexcept {
    constexpr auto long_bits = sizeof(unsigned long) * 8;

    if (bit >= sizeof(unsigned long) * 64)
        return false;

    const auto mask = 1UL << (bit % long_bits);
    return (arr[bit / long_bits] & mask) != 0;
}

void touch_injector::init_device() {
    uinput_user_dev dev{};
    input_id id{};

    ioctl(event_fd, EVIOCGID, &id);
    char name[UINPUT_MAX_NAME_SIZE] = "touch_injector";
    strncpy(dev.name, name, UINPUT_MAX_NAME_SIZE);
    dev.id = id;

    unsigned long evbits[(EV_MAX + 64) / 64]{};

    ioctl(event_fd, EVIOCGBIT(0, sizeof(evbits)), evbits);
    for (int i = 0; i <= EV_MAX; i++) {
        if (check_key_bit(i, evbits))
            ioctl(uinput_fd, UI_SET_EVBIT, i);
    }

    unsigned long absbits[(ABS_MAX + 64) / 64]{};
    ioctl(event_fd, EVIOCGBIT(EV_ABS, sizeof(absbits)), absbits);
    for (int code = 0; code <= ABS_MAX; code++) {
        if (!check_key_bit(code, absbits))
            continue;

        ioctl(uinput_fd, UI_SET_ABSBIT, code);

        input_absinfo info{};

        if (ioctl(event_fd, EVIOCGABS(code), &info) == 0) {
            dev.absmin[code] = info.minimum;
            dev.absmax[code] = info.maximum;
            dev.absfuzz[code] = info.fuzz;
            dev.absflat[code] = info.flat;
        }
    }

    ioctl(uinput_fd, UI_SET_PROPBIT, INPUT_PROP_DIRECT);

    write(uinput_fd, &dev, sizeof(dev));
    ioctl(uinput_fd, UI_DEV_CREATE);

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    ioctl(event_fd, EVIOCGRAB, 1);
}

void touch_injector::run_forward_worker() noexcept {
    if (!is_stopping)
        return;

    forward_event_worker.reset(new std::thread([this]() {
        input_event ev{};
        std::vector<input_event> evs;
        evs.reserve(16);

        while (!is_stopping.load()) {
            auto n = read(event_fd, &ev, sizeof(ev));
            if (n < 0) {
                LOGE("touch_injector::run_forward_worker read failed: %s",
                     strerror(errno));
                continue;
            }

            bool forward = true;

            if (ev.type == EV_ABS) {
                if (ev.code == ABS_MT_SLOT)
                    curr_real_slot = ev.value;

                if (curr_real_slot > max_forward_slot)
                    forward = false;
            }

            if (forward || ev.type == EV_SYN)
                evs.push_back(ev);

            if (ev.type == EV_SYN && ev.code == SYN_REPORT) {
                {
                    std::lock_guard lock{submit_mutex};
                    submit_data(evs);
                }
                evs.clear();
            }
        }
    }));
    is_stopping = false;
}

void touch_injector::submit_data(std::span<input_event> evs) noexcept {
    if (write(uinput_fd, evs.data(), evs.size_bytes()) == -1)
        LOGE("touch_injector::submit write failed: %s", strerror(errno));
}

void touch_injector::stop_forward_worker() noexcept {
    is_stopping = true;
    if (forward_event_worker)
        forward_event_worker->join();
}

bool touch_injector::is_running_forward() const noexcept {
    return !is_stopping.load();
}

void touch_injector::down(int slot, int x, int y) noexcept {
    const int send_slot = virtual_slot(slot);
    if (slot_down[send_slot])
        return;

    event_cache.emplace_back(timeval{}, EV_ABS, ABS_MT_SLOT, send_slot);
    event_cache.emplace_back(timeval{}, EV_ABS, ABS_MT_TRACKING_ID,
                             send_slot + 6000);
    event_cache.emplace_back(timeval{}, EV_ABS, ABS_MT_POSITION_X, x);
    event_cache.emplace_back(timeval{}, EV_ABS, ABS_MT_POSITION_Y, y);
    curr_event_cache_slot = send_slot;

    slot_down[send_slot] = true;
}

void touch_injector::move(int slot, int x, int y) noexcept {
    const int send_slot = virtual_slot(slot);
    if (!slot_down[send_slot])
        return;

    if (curr_event_cache_slot != send_slot) {
        event_cache.emplace_back(timeval{}, EV_ABS, ABS_MT_SLOT, send_slot);
        curr_event_cache_slot = send_slot;
    }
    event_cache.emplace_back(timeval{}, EV_ABS, ABS_MT_POSITION_X, x);
    event_cache.emplace_back(timeval{}, EV_ABS, ABS_MT_POSITION_Y, y);
}

void touch_injector::up(int slot) noexcept {
    const int send_slot = virtual_slot(slot);
    if (!slot_down[send_slot])
        return;

    if (curr_event_cache_slot != send_slot)
        event_cache.emplace_back(timeval{}, EV_ABS, ABS_MT_SLOT, send_slot);
    event_cache.emplace_back(timeval{}, EV_ABS, ABS_MT_TRACKING_ID, -1);
    curr_event_cache_slot = -1;

    slot_down[send_slot] = false;
}

void touch_injector::submit() noexcept {
    if (event_cache.empty())
        return;

    event_cache.emplace_back(timeval{}, EV_SYN, SYN_REPORT, 0);
    {
        std::lock_guard lock{submit_mutex};
        submit_data(event_cache);

        std::array<input_event, 2> restore{
            input_event{{}, EV_ABS, ABS_MT_SLOT, curr_real_slot},
            input_event{{}, EV_SYN, SYN_REPORT, 0},
        };
        curr_event_cache_slot = curr_real_slot;
        submit_data(restore);
    }
    event_cache.clear();
}

void touch_injector::cleanup_finger() noexcept {
    for (int i = 0; i < 10; i++) {
        up(i - max_forward_slot - 1);
        submit();
    }
}

int touch_injector::real_fd() const noexcept { return event_fd; }
int touch_injector::virtual_fd() const noexcept { return uinput_fd; }

bool touch_injector::is_virtual_down(int solt) const noexcept {
    return slot_down[virtual_slot(solt)];
}
int touch_injector::virtual_slot(int slot) const noexcept {
    return max_forward_slot + slot + 1;
}
} // namespace ap::touch::detail