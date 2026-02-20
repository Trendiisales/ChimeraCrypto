#pragma once
#include <cstdint>

namespace chimera {

#pragma pack(push, 1)
struct EventHeader
{
    uint64_t sequence;
    uint64_t timestamp_ns;
    uint32_t type;
    uint32_t payload_size;

    uint8_t reserved[128 - 8 - 8 - 4 - 4];
};
#pragma pack(pop)

static_assert(sizeof(EventHeader) == 128,
              "EventHeader must be 128 bytes");

}
