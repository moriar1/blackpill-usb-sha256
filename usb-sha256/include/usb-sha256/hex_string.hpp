#pragma once

#include <array>
#include <cstddef>
#include <cstdio>
#include <span>

#include "types.hpp"

namespace usbsha256 {

template <std::size_t TSize> class HexString {
public:
    using DataSpan = std::span<const Byte, TSize>;
    using StringSpan = std::span<const Byte, TSize * 2>;

    explicit HexString(DataSpan data);
    HexString(const HexString &) = default;
    HexString(HexString &&) = default;
    HexString &operator=(const HexString &) = default;
    HexString &operator=(HexString &&) = default;
    ~HexString() = default;

    StringSpan String() const;

private:
    std::array<Byte, TSize * 2> _string;
};

template <std::size_t TSize> HexString<TSize>::HexString(DataSpan data) {
    for (std::size_t i{}; i < TSize; ++i) {
        auto ptr = reinterpret_cast<char *>(_string.data() + i * 2);
        std::sprintf(ptr, "%02x", data[i]);
    }
}

template <std::size_t TSize> HexString<TSize>::StringSpan HexString<TSize>::String() const {
    return _string;
}

} // namespace usbsha256
