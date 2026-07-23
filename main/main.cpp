#include "ap/mem/map.h"
#include "ap/mem/mem.h"
#include "ap/mem/pid.h"
#include <iostream>


using namespace ap;

struct test {
    mem::offval<int, 0> head;
    mem::offval<int, 0x10> value;

    mem::offsettable _;
};

int main() {
    mem::pid pid{"bin.mt.plus"};
    mem::accessor reader{pid};
    mem::map64 map{pid};
    map.parse_only("libc.so");

    auto e = map.get_entries().at(0);

    test t;

    reader.read(e.start, t);

    std::cout << std::hex << "addr: " << e.start << ", " << std::dec << t.head
              << " " << t.value << std::endl;
}