#pragma once

#include <cstdint>
#include <pfr.hpp>
#include <type_traits>
#include <utility>


namespace ap::mem {
namespace detail {
template <typename T1, typename T2>
inline static constexpr bool is_same_v =
    std::is_same<std::remove_cvref_t<T1>, std::remove_cvref_t<T2>>::value;
}

template <typename T, std::uint16_t Offset> class offval {
    T val;

  public:
    using value_type = T;
    using reference = T &;
    using const_reference = const T &;

    offval() noexcept : val() {}

    template <typename Ty>
        requires detail::is_same_v<Ty, T>
    offval(Ty &&v) noexcept : val(std::forward<Ty>(v)) {}

    constexpr std::uint16_t offset() { return Offset; }

    operator reference() noexcept { return val; }
    operator const_reference() const noexcept { return val; }

    reference value() noexcept { return val; }
    const_reference value() const noexcept { return val; }
};

template <typename T, std::uint16_t Offset> class ptrval {
    T val;
    std::uintptr_t addr;

  public:
    using value_type = T;
    using reference = T &;
    using const_reference = const T &;
    using pointer = T *;
    using const_pointer = const T *;

    ptrval() noexcept : val(), addr(0) {}
    ptrval(std::uintptr_t addr) noexcept : val(), addr(addr) {}

    constexpr std::uint16_t offset() { return Offset; }

    std::uintptr_t &raddress() noexcept { return addr; }
    std::uintptr_t address() const noexcept { return addr; }

    reference value() noexcept { return val; }
    const_reference value() const noexcept { return val; }

    reference operator*() noexcept { return val; }
    const_reference operator*() const noexcept { return val; }

    pointer operator->() noexcept { return &val; }
    const_pointer operator->() const noexcept { return &val; }
};

struct offsettable {};

namespace detail {
template <typename Class, std::size_t Count>
struct is_offsettable
    : std::bool_constant<
          std::is_class_v<Class> &&
          std::is_same_v<offsettable, pfr::tuple_element_t<Count, Class>>> {};

template <typename T, std::size_t Count, bool End> struct is_offsettable_class {
  private:
    static constexpr std::size_t size = pfr::tuple_size_v<T>;

  public:
    static constexpr bool value =
        is_offsettable<T, Count>::value ||
        is_offsettable_class<T, Count + 1, Count == size - 1>::value;
};
template <typename T, std::size_t Count>
struct is_offsettable_class<T, Count, true> : std::false_type {};

template <typename T>
inline static constexpr bool is_offsettable_v =
    is_offsettable_class<T, 0, false>::value;
} // namespace detail
} // namespace ap::mem