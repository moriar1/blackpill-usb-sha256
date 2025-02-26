#include <wolfssl/wolfcrypt/sha256.h>

#include "sha256sum.hpp"

namespace crypt {

Sha256Sum::Sha256Sum() { wc_InitSha256(&_sha256); }

Sha256Sum::HashType Sha256Sum::Hash(SpanType data) {
    wc_Sha256Update(&_sha256, data.data(), data.size());
    wc_Sha256Final(&_sha256, _hash.data());
    return _hash;
}

} // namespace crypt
