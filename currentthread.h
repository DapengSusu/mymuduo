#pragma once

#define LIKELY(x) (__builtin_expect(!!(x), 1))
#define UNLIKELY(x) (__builtin_expect(!!(x), 0))

namespace current_thread {
    extern thread_local int t_cachedTid;

    // 通过Linux系统调用，获取当前线程tid
    void cacheTid();

    inline int tid()
    {
        if (__builtin_expect(t_cachedTid == 0, 0)) {
            cacheTid();
        }

        return t_cachedTid;
    }
}
