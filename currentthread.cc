#include "currentthread.h"

#include <unistd.h>
#include <sys/syscall.h>

namespace current_thread {
    // 当前线程tid
    thread_local int t_cachedTid = 0;

    void cacheTid()
    {
        if (t_cachedTid == 0) {
            t_cachedTid = static_cast<pid_t>(::syscall(SYS_gettid));
        }
    }
}