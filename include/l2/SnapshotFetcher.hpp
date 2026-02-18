#pragma once
#include <string>
#include <curl/curl.h>
#include "l2/L2Types.hpp"

namespace chimera {

class SnapshotFetcher {
public:
    static bool fetch(const std::string& symbol, Snapshot& out);

private:
    static size_t write_callback(void* contents, size_t size, size_t nmemb, void* userp);
    static bool parse_snapshot(const std::string& json, Snapshot& out);
};

}
