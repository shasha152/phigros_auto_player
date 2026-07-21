#include "ap/touch/touch_controller.h"
#include <chrono>
#include <iostream>
#include <thread>

#include "ap/mem/map.h"

int main() {
    // ap::touch::touch_controller touch_controller(3);
    // touch_controller.move(200, 200, 800, 800, 1000,
    //                       std::chrono::milliseconds(10));
    // touch_controller.move(150, 800, 0, 2000, 1000,
    //                       std::chrono::milliseconds(10));
    // touch_controller.move(700, 800, 1000, 20, 1000,
    //                       std::chrono::milliseconds(10));

    // while (touch_controller.has_tasks()) {
    //     touch_controller.update();
    //     std::this_thread::sleep_for(std::chrono::milliseconds(1));
    // }

    ap::mem::map64 map{1};
    map.parse_only("libil2cpp.so");

    for (const auto &e : map.get_entries())
        std::cout << e.start << " " << e.end << std::endl;
}