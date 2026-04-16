#pragma once

namespace ipc
{
    namespace socket
    {

#define Debug true
#if Debug
#define Exception
#else
#define Exception noexcept
#endif

#define TCP true
#define UDP false
    }
}