#pragma once

#include <atomic>
#include <linux/input.h>
#include <memory>
#include <span>
#include <thread>
#include <vector>

namespace ap::touch {
class virtual_touch {
    int uinput_fd;
    int event_fd;

    std::unique_ptr<std::thread> forward_event_worker;
    std::atomic_bool is_stopping = true;

    int max_forward_slot;
    int curr_real_slot = 0;
    std::vector<input_event> event_cache;
    int curr_event_cache_slot = 0;
    std::mutex submit_mutex;

    std::array<bool, 10> slot_down{};

  public:
    static std::unique_ptr<virtual_touch> create(int max_forward_slot);
    int real_fd() const noexcept;
    int virtual_fd() const noexcept;

    void run_forward_worker() noexcept;
    void stop_forward_worker() noexcept;
    bool is_running_forward() const noexcept;

    void down(int slot, int x, int y) noexcept;
    void move(int slot, int x, int y) noexcept;
    void up(int slot) noexcept;
    void submit() noexcept;

    ~virtual_touch();

  private:
    int scan_touch_device() const;
    bool check_key_bit(unsigned int bit,
                       const unsigned long *arr) const noexcept;
    void init_device();
    void submit_data(std::span<input_event> evs) noexcept;

    int virtual_slot(int slot) const noexcept;
    
    void cleanup_finger() noexcept;
};
} // namespace ap::touch