#include <wolfssl/wolfcrypt/sha256.h>

#include "sha256sum.hpp"
#include "types.hpp"

namespace usbsha256 {

Sha256Sum::Sha256Sum() { wc_InitSha256(&_sha256); }

Sha256Sum::HashSpan Sha256Sum::Hash(BytesSpan bytes) {
    wc_Sha256Update(&_sha256, bytes.data(), bytes.size());
    wc_Sha256Final(&_sha256, _hash.data());
    return _hash;
}

} // namespace usbsha256
