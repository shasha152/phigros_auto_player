#pragma once

#include <type_traits>
namespace ap::mem {
namespace detail {
class vm_rdwr;
}

template <typename Mem, typename = void> class basic_reader {
  public:
};

template <typename Mem>
class basic_reader<Mem,
                   std::enable_if_t<std::is_same_v<Mem, detail::vm_rdwr>>> {};

} // namespace ap::mem