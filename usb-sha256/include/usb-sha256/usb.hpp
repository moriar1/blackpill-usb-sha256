#pragma once

#include <atomic>
#include <string_view>

#include "types.hpp"
#include "fast_buffer.hpp"

namespace usbsha256 {

class Usb {
public:
    using Buffer = FastBuffer<1024>;
    static Usb &instance();
    // methods for receiving data
    void WaitReceiving();
    BytesSpan GetBuffer();
    void ClearBuffer();
    // transmission methods
    void Transmit(BytesSpan data);
    void Transmit(std::string_view text);

private:
    Usb();
    static void receive_callback(const Byte *const data, const std::uint32_t size);

private:
    Buffer buffer_{};
    std::atomic<bool> received_{};
};

} // namespace usbsha256
