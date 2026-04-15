/*
   Copyright 2026 Talon Kettuso & Contributors

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/
#pragma once
#include <stdint.h>

#define __bswap16(x) ({ \
    uint16_t __val = (x); \
    ((uint16_t)((((uint16_t)(__val) & 0xff00) >> 8) | \
                (((uint16_t)(__val) & 0x00ff) << 8))); \
})

#define __bswap32(x) ({ \
    uint32_t __val = (x); \
    ((uint32_t)((((uint32_t)(__val) & 0xff000000) >> 24) | \
                (((uint32_t)(__val) & 0x00ff0000) >> 8)  | \
                (((uint32_t)(__val) & 0x0000ff00) << 8)  | \
                (((uint32_t)(__val) & 0x000000ff) << 24))); \
})

#define __bswap64(x) ({ \
    uint64_t __val = (x); \
    ((uint64_t)((((uint64_t)(__val) & 0xff00000000000000ULL) >> 56) | \
                (((uint64_t)(__val) & 0x00ff000000000000ULL) >> 40) | \
                (((uint64_t)(__val) & 0x0000ff0000000000ULL) >> 24) | \
                (((uint64_t)(__val) & 0x000000ff00000000ULL) >> 8)  | \
                (((uint64_t)(__val) & 0x00000000ff000000ULL) << 8)  | \
                (((uint64_t)(__val) & 0x0000000000ff0000ULL) << 24) | \
                (((uint64_t)(__val) & 0x000000000000ff00ULL) << 40) | \
                (((uint64_t)(__val) & 0x00000000000000ffULL) << 56))); \
})

#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#define htobe16(x) (x)
#define htole16(x) __bswap16(x)
#define be16toh(x) (x)
#define le16toh(x) __bswap16(x)

#define htobe32(x) (x)
#define htole32(x) __bswap32(x)
#define be32toh(x) (x)
#define le32toh(x) __bswap32(x)

#define htobe64(x) (x)
#define htole64(x) __bswap64(x)
#define be64toh(x) (x)
#define le64toh(x) __bswap64(x)
#elif __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define htobe16(x) __bswap16(x)
#define htole16(x) (x)
#define be16toh(x) __bswap16(x)
#define le16toh(x) (x)

#define htobe32(x) __bswap32(x)
#define htole32(x) (x)
#define be32toh(x) __bswap32(x)
#define le32toh(x) (x)

#define htobe64(x) __bswap64(x)
#define htole64(x) (x)
#define be64toh(x) __bswap64(x)
#define le64toh(x) (x)
#endif