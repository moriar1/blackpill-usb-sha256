#pragma once

#include <atomic>
#include <string_view>

#include "board/usbd_cdc_if.h"
#include "types.hpp"

#include "fast_buffer.hpp"

namespace usbsha256 {

class Usb {
public:
    using Buffer = FastBuffer<APP_RX_DATA_SIZE>;
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
