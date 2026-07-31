#pragma once

#include "ap/mem/mem.h"
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <sys/types.h>
#include <vector>

namespace ap::il2cpp {

class method {
    std::uintptr_t handle;

  public:
    method() = default;
    explicit method(std::uintptr_t handle) noexcept : handle(handle) {}

    std::optional<std::int64_t> invoke(std::vector<std::int64_t> args) noexcept;
};

struct context {
    std::uintptr_t cache_memory;
    mem::accessor accessor;
    std::uintptr_t domain_addr;

    method get_type;
    method find_objects_type;
    std::uintptr_t string_class_addr;

    explicit context(pid_t pid) noexcept : accessor(pid) {}
};

struct object_querier {
    const context &_context;

    explicit object_querier(const context &__context) noexcept
        : _context(__context) {}

    std::vector<std::uintptr_t> operator()(std::string_view str) noexcept;
};

void init_ctx(pid_t pid, std::uintptr_t base) noexcept;
const context &get_context() noexcept;

class _class {
    std::uintptr_t handle;
    std::string cls_name;

  public:
    _class() = default;
    explicit _class(std::uintptr_t handle) noexcept : handle(handle) {}

    template <typename ObjectQuerier = object_querier>
    std::vector<std::uintptr_t> find_object(
        ObjectQuerier &&querier = ObjectQuerier(get_context())) noexcept {
        return querier(name());
    }

    method get_method(std::string_view str, size_t args_count) noexcept;

    std::uintptr_t address() const noexcept;
    std::string_view name() noexcept;
};

class assembly {
    std::uintptr_t handle;

  public:
    explicit assembly(std::uintptr_t handle) noexcept : handle(handle) {}

    static assembly create(std::string_view assembly) noexcept;

    _class get_class(std::string_view namesp, std::string_view name) noexcept;
};

} // namespace ap::il2cpp