#pragma once

#include <cstdint>
#include <string>

namespace dzIPC::common
{
    // 64-bit FNV-1a constants
    constexpr uint64_t kFnvOffsetBasis64 = 14695981039346656037ull;
    constexpr uint64_t kFnvPrime64 = 1099511628211ull;

    inline uint64_t fnv1a64(const std::string &data)
    {
        uint64_t hash = kFnvOffsetBasis64;
        for (unsigned char c : data)
        {
            hash ^= static_cast<uint64_t>(c);
            hash *= kFnvPrime64;
        }
        return hash;
    }
}