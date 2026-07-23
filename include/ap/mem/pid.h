#pragma once

#include "detail/pid.h"
#include <sys/types.h>
namespace ap::mem {
class pid : public detail::base_pid {

  public:
    template <typename Pid>
        requires detail::is_convertible_pid_v<Pid>
    explicit pid(Pid pid) : detail::base_pid(pid) {}

    operator pid_t() const noexcept { return get_pid(); }
};
} // namespace ap::mem