#pragma once

#include <atomic>
#include <linux/input.h>
#include <memory>
#include <span>
#include <thread>
#include <vector>

namespace ap::touch::detail {
class touch_injector {
    int uinput_fd;
    int event_fd;
    int max_forward_slot;
    int curr_real_slot = 0;
    int curr_event_cache_slot = 0;

    std::array<bool, 10> slot_down{};

    std::unique_ptr<std::thread> forward_event_worker;
    std::vector<input_event> event_cache;
    std::mutex submit_mutex;

    std::atomic_bool is_stopping = false;

  public:
    static std::unique_ptr<touch_injector>
    create(int max_forward_slot) noexcept;
    int real_fd() const noexcept;
    int virtual_fd() const noexcept;

    void run_forward_worker() noexcept;
    void stop_forward_worker() noexcept;
    bool is_running_forward() const noexcept;

    void down(int slot, int x, int y) noexcept;
    void move(int slot, int x, int y) noexcept;
    void up(int slot) noexcept;
    void submit() noexcept;

    bool is_virtual_down(int solt) const noexcept;

    ~touch_injector();

  private:
    int scan_touch_device() const;
    bool check_key_bit(unsigned int bit,
                       const unsigned long *arr) const noexcept;
    void init_device();
    void submit_data(std::span<input_event> evs) noexcept;

    int virtual_slot(int slot) const noexcept;

    void cleanup_finger() noexcept;
};
} // namespace ap::touch::detail