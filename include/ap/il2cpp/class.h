#pragma once

#include "ap/mem/mem.h"
#include "ap/mem/offset.h"
#include <cstdint>
#include <utility>
#include <vector>

namespace ap::il2cpp {

struct vector3 {
    float x, y, z;
};

struct vector2 {
    float x, y;
};

namespace detail {
struct pcamera {
    mem::offval<float[16], 0x100> matrix;
    mem::offsettable _;
};
} // namespace detail

struct camera {
    mem::ptrval<detail::pcamera, 0x10> camera;
    mem::offsettable _;

    inline static int width = 0;
    inline static int height = 0;

    vector2 to_screen_point(vector3 world) const noexcept {
        const auto &m = camera->matrix;
        // World -> Clip
        float clip_x = world.x * m[0] + world.y * m[4] + world.z * m[8] + m[12];

        float clip_y = world.x * m[1] + world.y * m[5] + world.z * m[9] + m[13];

        float clip_w =
            world.x * m[3] + world.y * m[7] + world.z * m[11] + m[15];

        if (clip_w <= 0.0001f)
            return {-1.f, -1.f};

        float ndc_x = clip_x / clip_w;
        float ndc_y = clip_y / clip_w;

        return {(ndc_x + 1.f) * 0.5f * width, (1.f - ndc_y) * 0.5f * height};
        // float w = m[3] * world.x + m[7] * world.y + m[15];
        // float camera_z = w + m[14] * 0.001f;

        // float ndc_x = (m[0] * world.x + m[4] * world.y + m[12]) / camera_z;
        // float ndc_y = (m[1] * world.x + m[5] * world.y + m[13]) / camera_z;
        // float x = (ndc_x + 1) * width / 2;
        // float y = (ndc_y + 1) * height / 2;
        // return vector2(x, height - y);
    }
};

template <typename T> struct list {
    std::vector<T> array;
    std::vector<std::uintptr_t> addrs;

    int size;
};

} // namespace ap::il2cpp

namespace ap::mem {

template <typename T> struct handle_traits<il2cpp::list<T>> {
    std::uintptr_t array_addr;

    template <typename Handle, typename... Args>
    void operator()(std::uintptr_t addr, il2cpp::list<T> &val,
                    const Handle &handle, Args &&...args) noexcept {
        handle_traits<int>()(addr + 0x18, val.size, handle,
                             std::forward<Args>(args)...);
        if (val.size == 0)
            return;

        handle_traits<std::uintptr_t>()(addr + 0x10, array_addr, handle,
                                        std::forward<Args>(args)...);

        val.array.resize(val.size);
        val.addrs.resize(val.size);
        for (int i = 0; i < val.size; i++) {
            std::uintptr_t &obj = val.addrs[i];
            handle_traits<std::uintptr_t>()(array_addr + 0x20 + i * 0x8, obj,
                                            handle,
                                            std::forward<Args>(args)...);

            handle_traits<T>()(obj, val.array[i], handle,
                               std::forward<Args>(args)...);
        }
    }
};
} // namespace ap::mem
