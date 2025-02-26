#include <algorithm>
#include <array>
#include <atomic>
#include <cstddef>
#include <cstdio>
#include <ranges>
#include <span>
#include <string_view>

#include <wolfssl/wolfcrypt/sha256.h>

#include "hex_string.hpp"
#include "sha256sum.hpp"
#include "shell.hpp"
#include "fast_buffer.hpp"
#include "types.hpp"
#include "usbd_cdc_if.h"

namespace usbsha256 {

using RxBuffer = FastBuffer<APP_RX_DATA_SIZE>;
using TxBuffer = FastBuffer<APP_TX_DATA_SIZE>;

RxBuffer receive_buffer{};
std::atomic<bool> receive_buffer_guard{};

void transmit(BytesSpan data) {
    if (data.size() > APP_TX_DATA_SIZE) {
        return;
    }
    auto ptr = const_cast<Byte *>(data.data());
    while (CDC_Transmit_FS(ptr, data.size()) == USBD_BUSY) {
    }
}

void transmit(std::string_view text) {
    auto ptr = reinterpret_cast<const Byte *>(text.data());
    BytesSpan data{ptr, text.size()};
    transmit(data);
}

void run() {
    Sha256Sum sha256{};

    while (true) {
        while (!receive_buffer_guard && !receive_buffer.Empty()) {
        }
        const auto lastByteIndex = receive_buffer.DataSize() - 1;
        const auto lastByte = receive_buffer.Data()[lastByteIndex];
        if (lastByte == '\r') {
            const auto data = receive_buffer.Data().first(lastByteIndex);
            const auto hash = sha256.Hash(data);
            const HexString<WC_SHA256_DIGEST_SIZE> hex{hash};
            transmit(hex.String());
            transmit("\r\n");
            receive_buffer.Clear();
        }
        receive_buffer_guard = false;
    }
}

} // namespace usbsha256

extern "C" {

void shell_run() { usbsha256::run(); }

void shell_receive_callback(unsigned char *const data, const uint32_t size) {
    using namespace usbsha256;
    if (receive_buffer_guard) {
        return;
    }
    receive_buffer.Append({data, size});
    receive_buffer_guard = true;
}
}
