#pragma once

#include "detail/pid.h"
#include "detail/vm.h"
#include "offset.h"

#include <cstdint>
#include <pfr.hpp>
#include <type_traits>

namespace ap::mem {

namespace detail {

template <typename Mem> class base_rdwr : public Mem {
  public:
    template <typename Pid>
        requires is_convertible_pid_v<Pid>
    explicit base_rdwr(Pid pid) noexcept : Mem(pid) {}

    template <typename T>
        requires(!is_offsettable_v<T>)
    Mem::return_type read(std::uintptr_t addr, T &value) noexcept {
        return Mem::read(addr, &value, sizeof(value));
    }

    template <typename T>
        requires(!is_offsettable_v<T>)
    Mem::return_type write(std::uintptr_t addr, T &value) noexcept {
        return Mem::write(addr, &value, sizeof(value));
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

} // namespace ap::mem