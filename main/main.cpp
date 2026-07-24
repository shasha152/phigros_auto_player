#include "ap/il2cpp/detail/symbol.h"
#include "ap/mem/map.h"
#include "ap/mem/mem.h"
#include "ap/mem/pid.h"
#include <cstdint>
#include <iostream>

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
    std::uintptr_t libil2cpp_base = map.get_entries().at(1).start;

    // for (auto e : map.get_entries()) {
    //     int head = 0;
    //     reader.read(e.start, head);
    //     if (head == 0x464C457F) {
    //         libil2cpp_base = e.start;
    //         break;
    //     }
    // }

    il2cpp::detail::symbol_caller::init_caller(pid, libil2cpp_base);
    auto addr = il2cpp::detail::symbol_caller::get_caller("il2cpp_domain_get")
                    ->invoke();
    std::cout << std::hex << "il2cpp_base: " << libil2cpp_base
              << " domain: " << addr.value_or(0ULL) << std::endl;
    // test t;

    // reader.read(e.start, t);

    // std::cout << std::hex << "addr: " << e.start << ", " << std::dec <<
    // t.head
    //           << " " << t.value << std::endl;
}