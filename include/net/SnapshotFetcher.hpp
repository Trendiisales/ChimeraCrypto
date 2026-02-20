#pragma once

#include <string>
#include "l2/L2Types.hpp"

namespace chimera {

class SnapshotFetcher {
public:
    bool fetch(const std::string& symbol,
               Snapshot& out_snapshot);

private:
    static size_t write_callback(void* contents,
                                 size_t size,
                                 size_t nmemb,
                                 void* userp);
};

}
