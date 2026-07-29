#include "ap/il2cpp/il2cpp.h"
#include "ap/mem/map.h"
#include "ap/mem/mem.h"
#include "ap/mem/pid.h"
#include <cstdint>
#include <iostream>
#include <ostream>
#include <type_traits>
#include <vector>

using namespace ap;

struct test {
    mem::offval<int, 0> head;
    mem::offval<int, 0x10> value;

    mem::offsettable _;
};

int main() {
    mem::pid pid{"com.PigeonGames.Phigros"};
    mem::accessor reader{pid};
    mem::map64 map{pid};
    map.parse_only("libil2cpp.so");
    auto module = map.get_module(reader);
    std::uintptr_t libil2cpp_base = module.has_value() ? module->start : 0;

    std::cout << std::hex << libil2cpp_base << std::endl;

    il2cpp::init_ctx(pid, libil2cpp_base);
    auto as = il2cpp::assembly::create("Assembly-CSharp.dll");

    for (auto obj : as.get_class("", "LevelControl").find_object()) {
        std::cout << obj << std::endl;
    }
}