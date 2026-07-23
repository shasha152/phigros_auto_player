#pragma once

#include "ap/touch/detail/touch_injector.h"
#include <array>
#include <cassert>
#include <chrono>
#include <ctime>
#include <list>
#include <memory>
#include <queue>

namespace ap::touch {

namespace detail {
using time_clock_t = std::chrono::steady_clock;

using time_point_t = std::chrono::time_point<time_clock_t>;

inline time_point_t get_time() noexcept { return time_clock_t::now(); }

template <typename Duration>
inline time_point_t get_time(const Duration &t) noexcept {
    return time_clock_t::now() + t;
}

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

        return -1;
    }

    void deallocate(int slot) noexcept { is_allocated_slot[slot] = false; }
};

} // namespace detail

struct finger_point {
    int x;
    int y;

    detail::time_point_t time;
    bool is_inject = false;
};

class touch_controller {
    struct finger_task {
        int slot;
        std::chrono::nanoseconds interval;
        std::queue<finger_point> points;
    };

    std::unique_ptr<detail::touch_injector> injector;

    detail::time_point_t now_time;

    std::list<finger_task> finger_tasks;
    detail::slot_allocator allocator;

  public:
    explicit touch_controller(int real_max_slot) noexcept
        : injector(detail::touch_injector::create(real_max_slot)),
          allocator(detail::MAX_SLOT - real_max_slot),
          now_time(detail::get_time()) {
        assert(injector);
        injector->run_forward_worker();
    }

    bool has_tasks() const noexcept { return !finger_tasks.empty(); }

    template <typename Duration> void down(int x, int y, const Duration &t) {
        int slot = allocator.allocate();
        assert(slot != -1);

        finger_tasks.push_back(
            finger_task{slot, t, finger_point{.x = x, .y = y}});
    }

    template <typename Duration>
    void move(int dx, int dy, int sx, int sy, int count, const Duration &st) {
        move(dx, dy, sx, sy, count, st, [](float x) { return x; });
    }

    template <typename Duration, typename Func>
    void move(int dx, int dy, int sx, int sy, int count, const Duration &st,
              const Func &func) {
        int slot = allocator.allocate();
        assert(slot != -1);

        const int dis_x = sx - dx;
        const int dis_y = sy - dy;

        std::queue<finger_point> points;

        for (int i = 0; i < count; i++) {
            float t = static_cast<float>(i) / (count - 1);
            points.push(finger_point{dx + static_cast<int>(dis_x * func(t)),
                                     dy + static_cast<int>(dis_y * func(t)),
                                     {}});
        }
        finger_tasks.emplace_back(slot, st, std::move(points));
    }

    void update() {
        now_time = detail::get_time();

        for (auto it = finger_tasks.begin(); it != finger_tasks.end();) {
            auto &[slot, interval, points] = *it;
            auto &front_point = points.front();

            if (!front_point.is_inject) {
                if (!injector->is_virtual_down(slot))
                    injector->down(slot, front_point.x, front_point.y);
                else
                    injector->move(slot, front_point.x, front_point.y);

                front_point.is_inject = true;
                front_point.time = now_time + interval;

            } else {
                if (now_time >= front_point.time) {
                    points.pop();

                    if (points.empty()) {
                        injector->up(slot);
                        it = finger_tasks.erase(it);

                        // 释放slot
                        allocator.deallocate(slot);

                        continue;
                    }
                }
            }

            it++;
        }

        injector->submit();
    }
};

} // namespace ap::touch