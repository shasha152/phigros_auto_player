#pragma once

#include "detail/vm.h"
#include "offset.h"

#include <cstdint>
#include <pfr.hpp>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

namespace ap::mem {

template <typename T> struct buffer_traits {
    std::pair<void *, std::size_t> operator()(T &val) noexcept {
        return {&val, sizeof(T)};
    }
};

namespace detail {

template <typename Mem> class base_rdwr : public Mem {
  public:
    using Mem::read;
    using Mem::write;
    template <typename Pid> explicit base_rdwr(Pid pid) noexcept : Mem(pid) {}

    template <typename T>
        requires(!is_offsettable_v<T>)
    Mem::return_type read(std::uintptr_t addr, T &value) noexcept {
        auto [buff, size] = buffer_traits<T>()(value);
        return Mem::read(addr, buff, size);
    }

    template <typename T>
        requires(!is_offsettable_v<T>)
    Mem::return_type write(std::uintptr_t addr, T &value) noexcept {
        auto [buff, size] = buffer_traits<T>()(value);
        return Mem::write(addr, buff, size);
    }
};
} // namespace detail

template <typename Mem, typename = void>
class basic_accessor : public detail::base_rdwr<Mem> {
  public:
    using detail::base_rdwr<Mem>::read;
    using detail::base_rdwr<Mem>::write;

    template <typename Pid>
    explicit basic_accessor(Pid pid) : detail::base_rdwr<Mem>(pid) {}

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
        return std::pair<void *, std::size_t>{val.data(),
                                              (val.size() + 1) * sizeof(CharT)};
    }
};

template <typename CharT, typename Traits>
struct buffer_traits<std::basic_string_view<CharT, Traits>> {
    auto operator()(std::basic_string_view<CharT, Traits> &val) noexcept {
        return std::pair<void *, std::size_t>{const_cast<CharT *>(val.data()),
                                              (val.size() + 1) * sizeof(CharT)};
    }
};

template <typename Tp, typename Allocator>
struct buffer_traits<std::vector<Tp, Allocator>> {
    auto operator()(std::vector<Tp, Allocator> &val) noexcept {
        return std::pair<void *, std::size_t>{val.data(),
                                              val.size() * sizeof(Tp)};
    }
};

template <typename T, std::size_t N> struct buffer_traits<T[N]> {
    auto operator()(T (&val)[N]) noexcept {
        return std::pair<void *, std::size_t>{static_cast<void *>(val),
                                              sizeof(T) * N};
    }
};

} // namespace ap::mem