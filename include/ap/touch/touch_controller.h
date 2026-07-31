#pragma once

#include "ap/meta/log.h"
#include "ap/meta/run_cmd.h"
#include "ap/touch/detail/touch_injector.h"
#include <algorithm>
#include <array>
#include <atomic>
#include <cassert>
#include <chrono>
#include <ctime>
#include <memory>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace ap::touch {

namespace detail {

inline static constexpr auto MAX_SLOT = 9;
class slot_allocator {
    int max_slot;
    std::array<bool, MAX_SLOT + 1> is_allocated_slot;

  public:
    explicit slot_allocator(int max_slot) noexcept : max_slot(max_slot) {
        is_allocated_slot.fill(false);
    }

    int allocate() noexcept {
        for (int slot = 0; slot <= max_slot; slot++) {
            if (!is_allocated_slot[slot]) {
                is_allocated_slot[slot] = true;
                return slot;
            }
        }

        LOGI("slot_allocator::allocate: 无手指可分配%d", max_slot);

        return -1;
    }

    void deallocate(int slot) noexcept { is_allocated_slot[slot] = false; }
};

} // namespace detail

class touch_controller {
    std::unique_ptr<detail::touch_injector> injector;

    detail::slot_allocator allocator;

    std::atomic<int> screen_orientation;
    int screen_height;
    int screen_width;

    std::thread dump_current_orientatio;
    std::atomic<bool> is_stop = false;

    std::vector<int> need_delete;

  public:
    explicit touch_controller(int real_max_slot) noexcept
        : injector(detail::touch_injector::create(real_max_slot)),
          allocator(detail::MAX_SLOT - real_max_slot) {
        assert(injector);
        injector->run_forward_worker();

        dump_current_orientatio = std::thread([this]() {
            while (!is_stop) {
                screen_orientation = std::stoi(meta::run_cmd(
                    "dumpsys display | grep 'mCurrentOrientation' | "
                    "cut -d'=' -f2"));

                std::this_thread::sleep_for(std::chrono::milliseconds(500));
            }
        });
    }

    ~touch_controller() {
        is_stop = true;

        if (dump_current_orientatio.joinable())
            dump_current_orientatio.join();
    }

    int down(int x, int y) noexcept {
        int slot = allocator.allocate();
        if (slot == -1)
            return slot;

        auto [rx, ry] = rotate_point(x, y);

        injector->down(slot, rx, ry);
        return slot;
    }

    void move(int slot, int x, int y) noexcept {
        if (slot == -1)
            return;
        auto [rx, ry] = rotate_point(x, y);
        injector->move(slot, rx, ry);
    }

    void up(int slot) noexcept {
        if (slot == -1)
            return;
        injector->up(slot);
        need_delete.push_back(slot);
    }

    void submit() noexcept {
        injector->submit();
        std::ranges::for_each(need_delete,
                              [this](int slot) { allocator.deallocate(slot); });
        need_delete.clear();
    }

    void set_screen_wh(int width, int height) noexcept {
        screen_width = width;
        screen_height = height;
    }

  private:
    std::pair<int, int> rotate_point(int x, int y) noexcept {
        int rotated_x = x;
        int rotated_y = y;

        switch (screen_orientation) {
        case 0:
            break;
        case 1:
            rotated_y = x;
            rotated_x = screen_width - y;
            break;
        case 2:
            rotated_x = screen_width - x;
            rotated_y = screen_height - y;
            break;
        case 3:
            rotated_y = screen_height - x;
            rotated_x = y;
            break;
        }
        return {rotated_x, rotated_y};
    }
};

} // namespace ap::touch