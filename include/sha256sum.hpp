#pragma once

#include <cstddef>
#include <cstdio>
#include <span>

#include <wolfssl/wolfcrypt/sha256.h>

#include "static_buffer.hpp"

namespace crypt {

using ElementType = static_buffer::ElementType;
using SpanType = static_buffer::SpanType;

class Sha256Sum {
public:
    using HashType = std::span<const ElementType, WC_SHA256_DIGEST_SIZE>;

    Sha256Sum();
    Sha256Sum(const Sha256Sum&) = default;
    Sha256Sum(Sha256Sum&&) = default;
    Sha256Sum& operator=(const Sha256Sum&) = default;
    Sha256Sum& operator=(Sha256Sum&&) = default;
    ~Sha256Sum() = default;

    HashType Hash(SpanType data) const;
private:
    wc_Sha256 sha256 = {};
    std::array<ElementType, WC_SHA256_DIGEST_SIZE> _hash;
};

template <std::size_t TSize>
class HexString {
public:
    using DataType = std::span<const ElementType, TSize>;
    using StringType = std::span<const ElementType, TSize * 2>;

    explicit HexString(DataType data);
    HexString(const HexString&) = default;
    HexString(HexString&&) = default;
    HexString& operator=(const HexString&) = default;
    HexString& operator=(HexString&&) = default;
    ~HexString() = default;

    StringType String() const;
private:
    std::array<ElementType, TSize * 2> _string;
};

template <std::size_t TSize>
HexString<TSize>::HexString(DataType data) {
    for (std::size_t i{}; i < TSize; ++i) {
        auto ptr = reinterpret_cast<char *>(_string.data() + i * 2);
        std::sprintf(ptr, "%02x", data[i]);
    }
}

template <std::size_t TSize>
HexString<TSize>::StringType HexString<TSize>::String() const {
    return _string;
}

}
