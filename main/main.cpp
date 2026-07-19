#include "ap/touch/virtual_touch.h"
#include <chrono>
#include <iostream>
#include <thread>

int main() {
    auto touch = ap::touch::virtual_touch::create(3);
    if (touch) {
        std::cout << "创建成功\n";
    }
    touch->run_forward_worker();
    touch->down(0, 100, 800);
    touch->submit();

    for (int x = 100; x < 800; x += 2) {
        touch->move(0, x, 800);
        touch->submit();

        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }

    touch->up(0);
    touch->submit();
}