#include "ap/il2cpp/il2cpp.h"
#include "ap/il2cpp/detail/symbol.h"
#include "ap/mem/mem.h"
#include "ap/meta/log.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <sys/mman.h>
#include <vector>

namespace ap::il2cpp {

using il2caller = detail::symbol_caller;

static std::unique_ptr<context> context_;

void init_ctx(pid_t pid, std::uintptr_t base) noexcept {
    detail::symbol_caller::init_caller(pid, base);

    auto mmap_addr = detail::get_libc_symbol("mmap");
    std::array<std::int64_t, 6> args = {0,
                                        0x1000,
                                        PROT_READ | PROT_WRITE | PROT_EXEC,
                                        MAP_PRIVATE | MAP_ANONYMOUS,
                                        -1,
                                        0};
    auto ret = il2caller::g_ptrace_controller.call(mmap_addr, args);
    if (!ret) {
        LOGI("init_ctx: call mmap failed");
        return;
    }

    context_ = std::make_unique<context>(pid);
    context_->cache_memory = ret.value();
    context_->domain_addr =
        il2caller::get_caller("il2cpp_domain_get")->invoke().value();

    context_->get_type = assembly::create("mscorlib.dll")
                             .get_class("System", "Type")
                             .get_method("GetType", 1);
    context_->find_objects_type = assembly::create("UnityEngine.CoreModule.dll")
                                      .get_class("UnityEngine", "Object")
                                      .get_method("FindObjectsOfType", 1);
    context_->string_class_addr = assembly::create("mscorlib.dll")
                                      .get_class("System", "String")
                                      .address();

    LOGI("init_ctx cache_memory: %lx", context_->cache_memory);
}

const context &get_context() noexcept { return *context_; }

std::vector<std::uintptr_t>
object_querier::operator()(std::string_view str) noexcept {
    std::vector<std::uintptr_t> res;
    std::u16string u16str =
        std::filesystem::path(str).u16string() + u",Assembly-CSharp";

    struct __t {
        std::uintptr_t cls;
        std::uintptr_t null;
        int size;
    } v{context_->string_class_addr, 0, static_cast<int>(u16str.size())};

    context_->accessor.write(context_->cache_memory, v);
    context_->accessor.write(context_->cache_memory + sizeof(__t) - 4, u16str);

    auto type = context_->get_type.invoke(
        {static_cast<std::int64_t>(context_->cache_memory)});
    auto object_array_addr =
        context_->find_objects_type.invoke({type.value_or(0L)});

    if (!object_array_addr) {
        LOGW("object_querier::operator(): %s not found", str.data());
        return res;
    }

    LOGI("object_querier::operator(): find addr %lx",
         object_array_addr.value());

    std::size_t objects_size = 0;
    context_->accessor.read(object_array_addr.value() + 0x18, objects_size);

    res.resize(objects_size);
    context_->accessor.read(object_array_addr.value() + 0x20, res);
    return res;
}

std::optional<std::int64_t>
method::invoke(std::vector<std::int64_t> args) noexcept {
    std::uintptr_t native_addr;

    context_->accessor.read(handle, native_addr);

    return il2caller::g_ptrace_controller.call(native_addr, args);
}

method _class::get_method(std::string_view str, size_t args_count) noexcept {
    // il2cpp_class_get_method_from_name

    context_->accessor.write(context_->cache_memory, str);

    auto mtd = il2caller::get_caller("il2cpp_class_get_method_from_name")
                   ->invoke(handle, context_->cache_memory, args_count);
    LOGI("_class::get_method: %s=%lx", str.data(), mtd.value_or(0UL));
    return method(mtd.value_or(0UL));
}

_class assembly::get_class(std::string_view namesp,
                           std::string_view name) noexcept {
    context_->accessor.write(context_->cache_memory,
                             const_cast<char *>(namesp.data()),
                             namesp.size() + 1);
    context_->accessor.write(context_->cache_memory + namesp.size() + 1,
                             const_cast<char *>(name.data()), name.size() + 1);

    auto cls = il2caller::get_caller("il2cpp_class_from_name")
                   ->invoke(handle, context_->cache_memory,
                            context_->cache_memory + namesp.size() + 1);

    return _class(cls.value());
}

std::string_view _class::name() noexcept {
    if (!cls_name.empty())
        return cls_name;

    char buffer[128] = {0};
    auto cls_name_addr =
        il2caller::get_caller("il2cpp_class_get_name")->invoke(handle);

    context_->accessor.read(cls_name_addr.value(), buffer);
    cls_name = buffer;

    return cls_name;
}

assembly assembly::create(std::string_view assembly_s) noexcept {
    size_t assemblies_size = 0;

    auto assemblies_array_addr =
        il2caller::get_caller("il2cpp_domain_get_assemblies")
            ->invoke(context_->domain_addr, context_->cache_memory)
            .value();
    context_->accessor.read(context_->cache_memory, assemblies_size);

    std::vector<std::uintptr_t> assemblies(assemblies_size);
    // context_->accessor.read(assemblies_array_addr, assemblies.data(),
    //                         assemblies.size() * sizeof(std::uintptr_t));
    context_->accessor.read(assemblies_array_addr, assemblies);
    // LOGI("assembly::create assemblies_array_size: %ld", assemblies.size());

    for (size_t i = 0; i < assemblies.size(); i++) {
        // if (assemblies[i] == 0)
        //     break;

        char name[128] = {0};

        auto image_addr = il2caller::get_caller("il2cpp_assembly_get_image")
                              ->invoke(assemblies[i])
                              .value();
        auto name_addr = il2caller::get_caller("il2cpp_image_get_name")
                             ->invoke(image_addr)
                             .value();

        context_->accessor.read(name_addr, name);
        if (std::string_view(name) == assembly_s)
            return assembly(image_addr);
    }

    LOGW("assembly::create: %s not found", assembly_s.data());

    return assembly(0);
}

std::uintptr_t _class::address() const noexcept { return handle; }

} // namespace ap::il2cpp