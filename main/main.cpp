#include "ap/auto_player.h"
#include "ap/mem/pid.h"
#include <chrono>
#include <thread>

using namespace ap;

struct test {
    mem::offval<int, 0> head;
    mem::offval<int, 0x10> value;

    mem::offsettable _;
};

int main() {
    mem::pid pid{"com.PigeonGames.Phigros"};
    auto_player player(pid, 2);

    player.init();
    player.set_fps(60);
    player.create();

    while (player.run()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    // mem::accessor reader{pid};
    // mem::map64 map{pid};
    // map.parse_only("libil2cpp.so");
    // auto module = map.get_module(reader);
    // std::uintptr_t libil2cpp_base = module.has_value() ? module->start : 0;

    // std::cout << std::hex << libil2cpp_base << std::endl;

    // il2cpp::init_ctx(pid, libil2cpp_base);
    // auto as = il2cpp::assembly::create("Assembly-CSharp.dll");

    // for (auto obj : as.get_class("", "LevelControl").find_object()) {

    //     detail::level_control level;
    //     reader.read(obj, level);

    //     LOGI("judge=%lx, judge_lines_addr=%lx", level.judgeControl.address(),
    //          level.judgeControl->judgeLineControls.address());
    //     LOGI("object=%lx, screenW=%f, screenH=%f, nowTime=%f", obj,
    //          level.screenW.value(), level.screenH.value(),
    //          level.judgeControl->nowTime.value());

    //     for (auto &v : level.judgeControl->judgeLineControls->array) {
    //         LOGI("index=%d, theta=%f, note_num=%d", v.index.value(),
    //              v.theta.value(), v.notesAbove->size + v.notesBelow->size);
    //     }

    //     for (auto &v : level.judgeControl->judgeLineControls->array.at(0)
    //                        .judgeLineMoveEvents->array) {
    //         LOGI("%s", mem::to_string(v).c_str());
    //     }
    // }

    // pfr::for_each_field_with_name(test{}, [](auto name, auto v, auto i) {
    //     std::cout << name << std::endl;
    // });
}