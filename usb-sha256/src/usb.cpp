#include "usb.hpp"

#include <cstdint>

#include "board/usbd_cdc_if.h"

#include "types.hpp"

namespace usbsha256 {

Usb &Usb::instance() {
    static Usb usb{};
    return usb;
}

Usb::Usb() { board_set_usb_receive_callback(&Usb::receive_callback); }

void Usb::WaitReceiving() {
    received_ = false;
    while (!received_) {
    }
}

BytesSpan Usb::GetBuffer() { return {buffer_.Data()}; }

void Usb::ClearBuffer() { buffer_.Clear(); }

void Usb::Transmit(BytesSpan data) {
    if (data.size() > APP_TX_DATA_SIZE) {
        return;
    }
    auto ptr = const_cast<Byte *>(data.data());
    while (CDC_Transmit_FS(ptr, data.size()) == USBD_BUSY) {
    }
}

void Usb::Transmit(std::string_view text) {
    auto ptr = reinterpret_cast<const Byte *>(text.data());
    BytesSpan data{ptr, text.size()};
    Transmit(data);
}

void Usb::receive_callback(const Byte *const data, const std::uint32_t size) {
    Usb &usb = instance();
    if (usb.received_) {
        return;
    }
    usb.buffer_.Append({data, size});
    usb.received_ = true;
}

} // namespace usbsha256
