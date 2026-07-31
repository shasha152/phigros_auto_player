#pragma once

#include "detail/vm.h"
#include "offset.h"

#include <cstddef>
#include <cstdint>
#include <pfr.hpp>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

namespace ap::mem {

template <typename T> struct buffer_traits {
    std::pair<T *, std::size_t> operator()(T &val) noexcept {
        return {&val, sizeof(T)};
    }
};

// template <typename Handle>
// using handle_traits_return_t =
//     std::invoke_result_t<Handle, std::uintptr_t, void *, size_t>;



template <typename T> struct handle_traits {
    template <typename Handle, typename... Args>
    void operator()(std::uintptr_t addr, T &val, const Handle &handle,
                    Args &&...args) noexcept {
        auto [buff, size] = buffer_traits<T>()(val);
        handle(addr, buff, size, std::forward<Args>(args)...);
    }
};

namespace detail {

template <typename Mem, typename Base> class base_rdwr : public Mem {
  public:
    using Mem::read;
    using Mem::write;
    template <typename Pid> explicit base_rdwr(Pid pid) noexcept : Mem(pid) {}

    template <typename T>
        requires(!is_offsettable_v<T>)
    void read(std::uintptr_t addr, T &value) noexcept {
        // auto [buff, size] = buffer_traits<T>()(value);
        // return Mem::read(addr, buff, size);

        return handle_traits<T>()(
            addr, value,
            [this]<typename Tp>(auto addr, Tp *buffer, auto size, auto self) {
                if constexpr (is_offsettable_v<Tp>)
                    static_cast<Base *>(self)->read(addr, *buffer);
                else
                    self->Mem::read(addr, buffer, size);
            },
            this);
    }

    template <typename T>
        requires(!is_offsettable_v<T>)
    void write(std::uintptr_t addr, T &value) noexcept {
        // auto [buff, size] = buffer_traits<T>()(value);
        // return Mem::write(addr, buff, size);
        return handle_traits<T>()(
            addr, value,
            [this]<typename Tp>(auto addr, Tp *buffer, auto size, auto self) {
                if constexpr (is_offsettable_v<Tp>)
                    static_cast<Base *>(self)->write(addr, *buffer);
                else
                    self->Mem::write(addr, buffer, size);
            },
            this);
    }
};
} // namespace detail

template <typename Mem>
class basic_accessor : public detail::base_rdwr<Mem, basic_accessor<Mem>> {
    using parent_type = detail::base_rdwr<Mem, basic_accessor<Mem>>;

  public:
    using parent_type::read;
    using parent_type::write;

    template <typename Pid>
    explicit basic_accessor(Pid pid) : parent_type(pid) {}

    template <typename T>
        requires detail::is_offsettable_v<T>
    void read(std::uintptr_t addr, T &value) noexcept {
        pfr::for_each_field(value, [this, addr]<typename Ty>(Ty &v) {
            if constexpr (!std::is_same_v<Ty, offsettable>)
                read_off_ptr_val(addr, v);
        });
    }

    template <typename T>
        requires detail::is_offsettable_v<T>
    void write(std::uintptr_t addr, T &value) noexcept {
        pfr::for_each_field(value, [this, addr]<typename Ty>(Ty &v) {
            if constexpr (!std::is_same_v<Ty, offsettable>)
                write_off_ptr_val(addr, v);
        });
    }

  private:
    template <typename T, std::uint16_t Offset>
    void read_off_ptr_val(std::uintptr_t addr, offval<T, Offset> &fv) noexcept {
        read(addr + Offset, fv.value());
    }

    template <typename T, std::uint16_t Offset>
    void read_off_ptr_val(std::uintptr_t addr, ptrval<T, Offset> &fv) noexcept {
        read(addr + Offset, fv.raddress());
        read(fv.address(), fv.value());
    }

    template <typename T, std::uint16_t Offset>
    void write_off_ptr_val(std::uintptr_t addr,
                           offval<T, Offset> &fv) noexcept {
        write(addr + Offset, fv.value());
    }

    template <typename T, std::uint16_t Offset>
    void write_off_ptr_val(std::uintptr_t addr,
                           ptrval<T, Offset> &fv) noexcept {
        write(addr + Offset, fv.raddress());
        // write(fv.address(), fv.value());
    }
};

using accessor = basic_accessor<detail::vm_rdwr>;

// template <typename Mem>
// class basic_reader<Mem, std::enable_if_t<std::is_same_v<Mem,
// detail::vm_rdwr>>>
//     : public detail::base_rdwr<Mem> {
//   public:
//     template <typename Pid>
//     explicit basic_reader(Pid pid) : detail::base_rdwr<Mem>(pid) {
//     }
// };

// template <typename Traits, typename Allocator>
template <typename CharT, typename Traits, typename Allocator>
struct buffer_traits<std::basic_string<CharT, Traits, Allocator>> {
    auto operator()(std::basic_string<CharT, Traits, Allocator> &val) noexcept {
        return std::pair<CharT *, std::size_t>{val.data(), (val.size() + 1) *
                                                               sizeof(CharT)};
    }
};

template <typename CharT, typename Traits>
struct buffer_traits<std::basic_string_view<CharT, Traits>> {
    auto operator()(std::basic_string_view<CharT, Traits> &val) noexcept {
        return std::pair<CharT *, std::size_t>{
            const_cast<CharT *>(val.data()), (val.size() + 1) * sizeof(CharT)};
    }
};

template <typename Tp, typename Allocator>
struct buffer_traits<std::vector<Tp, Allocator>> {
    auto operator()(std::vector<Tp, Allocator> &val) noexcept {
        return std::pair<Tp *, std::size_t>{val.data(),
                                            val.size() * sizeof(Tp)};
    }
};

template <typename T, std::size_t N> struct buffer_traits<T[N]> {
    auto operator()(T (&val)[N]) noexcept {
        return std::pair<T *, std::size_t>{static_cast<T *>(val),
                                           sizeof(T) * N};
    }
};

} // namespace ap::mem