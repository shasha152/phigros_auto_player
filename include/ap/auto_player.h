#pragma once

#include "ap/il2cpp/il2cpp.h"
#include "ap/mem/map.h"
#include "ap/mem/mem.h"
#include "ap/meta/log.h"
#include "il2cpp/class.h"
#include "mem/offset.h"
#include "touch/touch_controller.h"
#include <algorithm>
#include <array>
#include <cassert>
#include <cstdint>
#include <list>
#include <math.h>
#include <queue>
#include <ranges>

#include <sys/types.h>
#include <utility>
#include <vector>

#include <chrono>
#include <string_view>

class timer {
  public:
    explicit timer(std::string_view name = "") noexcept
        : name_(name), start_(clock::now()) {}

    ~timer() noexcept { LOGI("%s: %lfms", name_.data(), elapsed_ms()); }

    void reset() noexcept { start_ = clock::now(); }

    double elapsed_ms() const noexcept {
        return std::chrono::duration<double, std::milli>(clock::now() - start_)
            .count();
    }

    double elapsed_us() const noexcept {
        return std::chrono::duration<double, std::micro>(clock::now() - start_)
            .count();
    }

    double elapsed_ns() const noexcept {
        return std::chrono::duration<double, std::nano>(clock::now() - start_)
            .count();
    }

  private:
    using clock = std::chrono::steady_clock;

    std::string_view name_;
    clock::time_point start_;
};

namespace ap {
namespace detail {

// struct judge_line_event {
//     mem::offval<float, 0x10> startTime; // 0x10
//     mem::offval<float, 0x12> endTime;   // 0x14
//     mem::offval<float, 0x18> start;     // 0x18
//     mem::offval<float, 0x1C> end;       // 0x1C
//     mem::offval<float, 0x20> start2;    // 0x20
//     mem::offval<float, 0x24> end2;      // 0x24
//     mem::offsettable _;
// };

struct judge_line_event {
    std::array<char, 0x10> _;

    float startTime; // 0x10
    float endTime;   // 0x14
    float start;     // 0x18
    float end;       // 0x1C
    float start2;    // 0x20
    float end2;      // 0x24
};

enum class note_type : int { click = 1, flick = 2, hold = 3, drag = 4 };

struct chart_note {
    mem::offval<note_type, 0x10> type;
    mem::offval<float, 0x18> positionX;
    mem::offval<float, 0x1C> holdTime;
    mem::offval<float, 0x2C> realTime;

    mem::offval<int, 0x30> judgeLineIndex; // 0x30

    mem::offsettable _;
};

struct judge_line_control {
    mem::offval<int, 0x80> index;
    mem::offval<float, 0x84> theta;

    mem::ptrval<il2cpp::list<chart_note>, 0xA0> notesAbove;
    mem::ptrval<il2cpp::list<chart_note>, 0xA8> notesBelow;
    mem::ptrval<il2cpp::list<judge_line_event>, 0xB8> judgeLineMoveEvents;

    mem::offsettable _;
};

struct judge_control {
    mem::ptrval<il2cpp::list<judge_line_control>, 0x28>
        judgeLineControls;            // 0x28
    mem::offval<float, 0x40> nowTime; // 0x40

    mem::offsettable _;
};

struct level_control {
    mem::ptrval<judge_control, 0x38> judgeControl; // 0x38

    mem::ptrval<il2cpp::camera, 0xC8> backgroundCamera; // 0xC8
    mem::offval<float, 0x138> screenW;                  // 0x138
    mem::offval<float, 0x13C> screenH;                  // 0x13C

    mem::offsettable _;
};

} // namespace detail

inline static bool is_init_il2cpp = false;

struct note {
    struct point {
        float time;
        il2cpp::vector2 position;
        il2cpp::vector2 center;
    };

    int judge_index;
    std::queue<point> screen;

    detail::note_type type;

    int slot = -1;
};

class auto_player {
    int fps;

    mem::accessor accessor;
    detail::level_control level_control;

    il2cpp::_class level_control_class;
    std::uintptr_t il2cpp_base;

    std::list<note> builded_notes;

    touch::touch_controller touch_controller;

  public:
    explicit auto_player(pid_t pid, int real_finger_max) noexcept
        : fps(0), accessor(pid), touch_controller(real_finger_max) {}

    void init() {
        il2cpp_base = mem::map64(accessor.get_pid())
                          .parse_only("libil2cpp.so")
                          .get_module(accessor)
                          ->start;

        if (!is_init_il2cpp) {
            il2cpp::init_ctx(accessor.get_pid(), il2cpp_base);
            is_init_il2cpp = true;
        }

        level_control_class = il2cpp::assembly::create("Assembly-CSharp.dll")
                                  .get_class("", "LevelControl");
    }

    bool create() {
        auto lv_obj_addrs = level_control_class.find_object();

        if (lv_obj_addrs.empty())
            return false;

        accessor.read(lv_obj_addrs.at(0), level_control);
        LOGI("camera: %lx", level_control.backgroundCamera.address());

        il2cpp::camera::width = level_control.screenW;
        il2cpp::camera::height = level_control.screenH;

        touch_controller.set_screen_wh(level_control.screenH,
                                       level_control.screenW);

        LOGI("width=%f, height=%f", level_control.screenW.value(),
             level_control.screenH.value());
        build();
        return true;
    }

    bool run() {
        update_data();

        for (auto it = builded_notes.begin(); it != builded_notes.end();) {
            auto &n = *it;
            if (n.screen.empty()) {
                it = builded_notes.erase(it);
                touch_controller.up(n.slot);

                continue;
            }
            while (!n.screen.empty() &&
                   n.screen.front().time <=
                       level_control.judgeControl->nowTime) {

                auto &[_, orgpos, center] = n.screen.front();
                auto theta = -(level_control.judgeControl->judgeLineControls
                                   ->array[n.judge_index]
                                   .theta);
                auto pos = rotate(orgpos, center, theta * M_PI / 180);

                if (n.slot == -1)
                    n.slot = touch_controller.down(pos.x, pos.y);
                else
                    touch_controller.move(n.slot, pos.x, pos.y);

                n.screen.pop();
            }
            it++;
        }

        touch_controller.submit();

        return !builded_notes.empty();
    }

    void clear() noexcept { builded_notes.clear(); }

    void set_fps(int fps) { this->fps = fps; }

  private:
    void update_data() {
        auto &judge_control = level_control.judgeControl;
        judge_control->nowTime.update(accessor, judge_control.address());

        auto &judge_line_controls = judge_control->judgeLineControls;
        for (auto [val, addr] : std::views::zip(judge_line_controls->array,
                                                judge_line_controls->addrs))
            val.theta.update(accessor, addr);
    }

    auto integrate_notes(auto &lines) {
        auto notes =
            std::views::iota(std::size_t{0}, lines.size()) |
            std::views::transform([&](std::size_t line_index) {
                auto &line = lines[line_index];

                return std::array{std::views::all(line.notesBelow->array),
                                  std::views::all(line.notesAbove->array)} |
                       std::views::join |
                       std::views::transform([line_index](auto &note) {
                           if (note.type == detail::note_type::drag)
                               note.realTime.value() -= 0.05f;
                           return std::pair<std::size_t, detail::chart_note>{
                               line_index, note};
                       });
            }) |
            std::views::join | std::ranges::to<std::vector>();

        std::ranges::stable_sort(
            notes, {}, [](const auto &v) { return v.second.realTime; });
        return std::list<typename decltype(notes)::value_type>(notes.begin(),
                                                               notes.end());
    }

    void build() {
        auto &lines = level_control.judgeControl->judgeLineControls->array;
        auto list_notes = integrate_notes(lines);

        for (auto it = list_notes.begin(); it != list_notes.end();) {
            auto &[i, v] = *it;
            auto &line = lines[i];

            auto &events = line.judgeLineMoveEvents->array;
            auto evit = std::ranges::find_if(events, [&v](auto &ev) {
                return v.realTime >= ev.startTime && v.realTime < ev.endTime;
            });

            if (evit != events.end()) {
                auto line_pos = get_event_position(v.realTime, *evit);
                note note;

                note.judge_index = i;
                note.type = v.type;

                if (v.type == detail::note_type::hold) {
                    float interval = 1.0f / fps;

                    for (float start = 0; start < v.holdTime;
                         start += interval) {
                        float real = v.realTime + start;
                        auto evit2 =
                            std::find_if(evit, events.end(), [real](auto &ev) {
                                return real >= ev.startTime &&
                                       real < ev.endTime;
                            });
                        if (evit2 == events.end())
                            break;

                        auto line_pos_2 = get_event_position(real, *evit2);
                        auto screen_pos =
                            level_control.backgroundCamera->to_screen_point(
                                {line_pos_2.x + v.positionX, line_pos_2.y,
                                 0.0f});
                        auto center_pos =
                            level_control.backgroundCamera->to_screen_point(
                                {line_pos_2.x, line_pos_2.y, 0.0f});
                        note.screen.push({real, screen_pos, center_pos});
                    }
                } else if (v.type == detail::note_type::drag) {
                    il2cpp::vector3 note_pos{line_pos.x + v.positionX,
                                             line_pos.y, 0.0f};
                    auto screen_pos =
                        level_control.backgroundCamera->to_screen_point(
                            note_pos);
                    auto center_pos =
                        level_control.backgroundCamera->to_screen_point(
                            {line_pos.x, line_pos.y, 0.0f});

                    for (int i = 0; i < 20; i++) {
                        screen_pos.y -= i;

                        note.screen.push(
                            {v.realTime + i * 0.005f, screen_pos, center_pos});
                    }

                } else {
                    il2cpp::vector3 note_pos{line_pos.x + v.positionX,
                                             line_pos.y, 0.0f};
                    auto screen_pos =
                        level_control.backgroundCamera->to_screen_point(
                            note_pos);
                    auto center_pos =
                        level_control.backgroundCamera->to_screen_point(
                            {line_pos.x, line_pos.y, 0.0f});
                    note.screen.push({v.realTime, screen_pos, center_pos});
                }
                builded_notes.emplace_back(std::move(note));
            }

            it++;
        }
    }

    il2cpp::vector2 get_event_position(float realTime,
                                       const detail::judge_line_event &ev) {
        float t = (realTime - ev.startTime) / (ev.endTime - ev.startTime);
        il2cpp::vector2 line_pos{
            .x = ev.start + t * (ev.end - ev.start),
            .y = ev.start2 + t * (ev.end2 - ev.start2),
        };
        return line_pos;
    }

    il2cpp::vector2 rotate(il2cpp::vector2 p, il2cpp::vector2 center,
                           float rad) noexcept {
        float c = std::cos(rad);
        float s = std::sin(rad);

        float x = p.x - center.x;
        float y = p.y - center.y;

        return {
            .x = center.x + x * c - y * s,
            .y = center.y + x * s + y * c,
        };
    }
};
} // namespace ap