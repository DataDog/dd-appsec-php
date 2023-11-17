// Unless explicitly stated otherwise all files in this repository are
// dual-licensed under the Apache-2.0 License or BSD-3-Clause License.
//
// This product includes software developed at Datadog
// (https://www.datadoghq.com/). Copyright 2021 Datadog, Inc.

#include "compression.hpp"
#include <cstdint>
#include <string>
#include <zlib.h>

namespace dds {

static constexpr int64_t encoding = -0xf;
static constexpr int max_round_decompression = 100;

// Taken from PHP approach
// https://heap.space/xref/PHP-7.3/ext/zlib/php_zlib.h?r=8d3f8ca1#36
constexpr size_t COMPRESSED_SIZE_ESTIMATION(size_t in_len)
{
    // NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
    return (((size_t)((double)in_len * (double)1.015)) + 10 + 8 + 4 + 1);
    // NOLINTEND(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
}

// The implementation of this function is based on how PHP does
//  https://heap.space/xref/PHP-7.3/ext/zlib/zlib.c?r=9afce019#336
std::optional<std::string> compress(const std::string &text)
{
    std::string ret_string;
    z_stream strm = {};

    if (text.length() == 0) {
        return std::nullopt;
    }

    if (Z_OK == deflateInit2(&strm, -1, Z_DEFLATED, encoding, MAX_MEM_LEVEL,
                    Z_DEFAULT_STRATEGY)) {
        ret_string.resize(COMPRESSED_SIZE_ESTIMATION(text.length()), '\0');

        strm.next_in = (uint8_t *)&text[0];
        strm.next_out = (uint8_t *)&ret_string[0];
        strm.avail_in = text.length();
        strm.avail_out = ret_string.capacity();

        if (Z_STREAM_END == deflate(&strm, Z_FINISH)) {
            deflateEnd(&strm);
            /* size buffer down to actual length */
            ret_string.resize(strm.total_out);
            ret_string.shrink_to_fit();
            return ret_string;
        }
        deflateEnd(&strm);
    }
    return std::nullopt;
}

// Taken from PHP approach
// https://heap.space/xref/PHP-7.3/ext/zlib/zlib.c?r=9afce019#422
std::optional<std::string> uncompress(const std::string &compressed)
{
    int round = 0;
    size_t used = 0;
    size_t free;
    size_t capacity;

    if (compressed.length() < 1) {
        return std::nullopt;
    }

    z_stream strm = {};
    if (Z_OK != inflateInit2(&strm, encoding)) {
        return std::nullopt;
    }

    strm.next_in = (Bytef *)&compressed[0];
    strm.avail_in = compressed.length();
    std::string output;
    int status = Z_OK;

    capacity = strm.avail_in;
    output.resize(capacity);
    while ((Z_BUF_ERROR == status || (Z_OK == status && strm.avail_in > 0)) &&
           ++round < max_round_decompression) {
        strm.avail_out = free = capacity - used;
        strm.next_out = (Bytef *)&output[0] + used;
        status = inflate(&strm, Z_NO_FLUSH);
        used += free - strm.avail_out;
        capacity += (capacity >> 3) + 1;
        output.resize(capacity);
    }
    if (status == Z_STREAM_END) {
        inflateEnd(&strm);
        output.resize(used);
        output.shrink_to_fit();
        return output;
    }
    inflateEnd(&strm);

    return std::nullopt;
}

} // namespace dds
