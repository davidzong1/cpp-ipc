#pragma once
#include <thread>
#include <string>

#if defined(_WIN32)
#include <windows.h>
#elif defined(__linux__) || defined(__APPLE__)
#include <pthread.h>
#include <sched.h>
#include <cstring>
#endif

namespace dzIPC::ThreadDispatch
{
    inline bool set_thread_priority(std::thread *th, int prio, bool verbose, const std::string &name)
    {
        if (!th || !th->joinable())
            return false;

#if defined(_WIN32)
        int win_prio = THREAD_PRIORITY_ABOVE_NORMAL; // 可按 prio 映射
        BOOL ok = SetThreadPriority(th->native_handle(), win_prio);
        return ok != 0;

#elif defined(__linux__) || defined(__APPLE__)
        sched_param sp{};
        sp.sched_priority = prio;
        int rc = pthread_setschedparam(th->native_handle(), SCHED_FIFO, &sp);
        if (rc != 0 && verbose)
        {
            printf("\033[33m[Warning] Failed to set thread priority for thread '%s' with error: %s\033[0m\n",
                   name.c_str(), std::strerror(rc));
        }
        return rc == 0;

#else
        (void)prio;
        (void)verbose;
        (void)name;
        printf("\033[33m[Warning] Setting thread priority is not supported on this platform for thread '%s'\033[0m\n",
               name.c_str());
        return false;
#endif
    }
}