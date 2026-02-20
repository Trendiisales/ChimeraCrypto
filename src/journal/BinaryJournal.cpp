#include "journal/BinaryJournal.hpp"
#include <chrono>
#include <cstring>

namespace chimera {

BinaryJournal::BinaryJournal(const std::string& filename)
{
    file_.open(filename,
               std::ios::binary |
               std::ios::out |
               std::ios::app);
}

void BinaryJournal::append(uint64_t sequence,
                           uint32_t type,
                           const void* payload,
                           uint32_t payload_size)
{
    if (!file_.is_open())
        return;

    EventHeader header{};
    header.sequence = sequence;

    auto now = std::chrono::high_resolution_clock::now();
    header.timestamp_ns =
        std::chrono::duration_cast<std::chrono::nanoseconds>(
            now.time_since_epoch()).count();

    header.type = type;
    header.payload_size = payload_size;

    file_.write(reinterpret_cast<const char*>(&header),
                sizeof(header));

    file_.write(reinterpret_cast<const char*>(payload),
                payload_size);

    file_.flush();
}

void BinaryJournal::close()
{
    if (file_.is_open())
        file_.close();
}

}
