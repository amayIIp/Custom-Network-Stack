#pragma once
#include <cstdint>
#include <bit>

namespace itch {

inline uint16_t bswap16(uint16_t val) {
#if defined(__GNUC__) || defined(__clang__)
    return __builtin_bswap16(val);
#elif defined(_MSC_VER)
    return _byteswap_ushort(val);
#else
    return (val >> 8) | (val << 8);
#endif
}

inline uint32_t bswap32(uint32_t val) {
#if defined(__GNUC__) || defined(__clang__)
    return __builtin_bswap32(val);
#elif defined(_MSC_VER)
    return _byteswap_ulong(val);
#else
    return ((val >> 24) & 0xff) |
           ((val >> 8) & 0xff00) |
           ((val << 8) & 0xff0000) |
           ((val << 24) & 0xff000000);
#endif
}

inline uint64_t bswap64(uint64_t val) {
#if defined(__GNUC__) || defined(__clang__)
    return __builtin_bswap64(val);
#elif defined(_MSC_VER)
    return _byteswap_uint64(val);
#else
    return ((val >> 56) & 0xffULL) |
           ((val >> 40) & 0xff00ULL) |
           ((val >> 24) & 0xff0000ULL) |
           ((val >> 8) & 0xff000000ULL) |
           ((val << 8) & 0xff00000000ULL) |
           ((val << 24) & 0xff0000000000ULL) |
           ((val << 40) & 0xff000000000000ULL) |
           ((val << 56) & 0xff00000000000000ULL);
#endif
}

inline uint16_t be16(uint16_t val) {
    if constexpr (std::endian::native == std::endian::little) {
        return bswap16(val);
    }
    return val;
}

inline uint32_t be32(uint32_t val) {
    if constexpr (std::endian::native == std::endian::little) {
        return bswap32(val);
    }
    return val;
}

inline uint64_t be64(uint64_t val) {
    if constexpr (std::endian::native == std::endian::little) {
        return bswap64(val);
    }
    return val;
}

inline uint64_t read_ts48(const uint8_t* p) {
    return (static_cast<uint64_t>(p[0]) << 40) |
           (static_cast<uint64_t>(p[1]) << 32) |
           (static_cast<uint64_t>(p[2]) << 24) |
           (static_cast<uint64_t>(p[3]) << 16) |
           (static_cast<uint64_t>(p[4]) << 8)  |
           p[5];
}

} // namespace itch
