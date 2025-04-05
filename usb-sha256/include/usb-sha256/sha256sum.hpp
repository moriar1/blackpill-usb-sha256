#pragma once

#include <span>

#include <wolfssl/wolfcrypt/sha256.h>

#include "types.hpp"

namespace usbsha256 {

class Sha256Sum {
public:
    using HashSpan = std::span<const Byte, WC_SHA256_DIGEST_SIZE>;

    Sha256Sum();
    Sha256Sum(const Sha256Sum &) = default;
    Sha256Sum(Sha256Sum &&) = default;
    Sha256Sum &operator=(const Sha256Sum &) = default;
    Sha256Sum &operator=(Sha256Sum &&) = default;
    ~Sha256Sum() = default;

    HashSpan Hash(BytesSpan bytes);

private:
    wc_Sha256 _sha256 = {};
    std::array<Byte, WC_SHA256_DIGEST_SIZE> _hash;
};

} // namespace usbsha256
