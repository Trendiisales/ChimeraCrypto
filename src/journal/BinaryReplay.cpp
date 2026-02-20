#include "journal/BinaryReplay.hpp"
#include <vector>

namespace chimera {

BinaryReplay::BinaryReplay(const std::string& filename)
{
    file_.open(filename,
               std::ios::binary |
               std::ios::in);
}

void BinaryReplay::replay(
    std::function<void(const EventHeader&,
                       const std::vector<char>&)> handler)
{
    if (!file_.is_open())
        return;

    while (true)
    {
        EventHeader header{};
        file_.read(reinterpret_cast<char*>(&header),
                   sizeof(header));

        if (!file_)
            break;

        std::vector<char> payload(header.payload_size);
        file_.read(payload.data(),
                   header.payload_size);

        if (!file_)
            break;

        handler(header, payload);
    }
}

}
