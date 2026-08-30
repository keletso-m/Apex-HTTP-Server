#include "http_parser.h"
#include <string>
#include <cstdint>
#include <cstddef>

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
    std::string raw(reinterpret_cast<const char*>(data), size);

    HttpRequest req = HttpParser::parse(raw);
    (void)req;

    return 0;
}