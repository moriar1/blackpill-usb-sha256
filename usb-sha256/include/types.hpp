#pragma once

#include <cstdint>
#include <span>

namespace usbsha256 {

using Byte = std::uint8_t;
using BytesSpan = std::span<const Byte>;

} // namespace usbsha256
