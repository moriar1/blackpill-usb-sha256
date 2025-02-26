#include <algorithm>
#include <array>
#include <atomic>
#include <cstddef>
#include <cstdio>
#include <ranges>
#include <span>
#include <string_view>

#include <wolfssl/wolfcrypt/sha256.h>

#include "shell.h"
#include "static_buffer.hpp"
#include "usbd_cdc_if.h"

namespace shell {

using RxBuffer = static_buffer::Buffer<APP_RX_DATA_SIZE>;
using TxBuffer = static_buffer::Buffer<APP_TX_DATA_SIZE>;
using static_buffer::ElementType;
using static_buffer::SpanType;

RxBuffer receive_buffer{};
std::atomic<bool> receive_buffer_guard{};

void transmit(SpanType data) {
    if (data.size() > APP_TX_DATA_SIZE) {
        return;
    }
    auto ptr = const_cast<unsigned char *>(data.data());
    while (CDC_Transmit_FS(ptr, data.size()) == USBD_BUSY) {
    }
}

void transmit(std::string_view text) {
    auto ptr = reinterpret_cast<const ElementType *>(text.data());
    SpanType data{ptr, text.size()};
    transmit(data);
}

void run() {
    wc_Sha256 sha256{};
    wc_InitSha256(&sha256);

    while (true) {
        while (!receive_buffer_guard && !receive_buffer.Empty()) {
        }
        const auto lastElementIndex = receive_buffer.DataSize() - 1;
        const auto lastElement = receive_buffer.Data()[lastElementIndex];
        if (lastElement == '\r') {
            const auto data = receive_buffer.Data().first(lastElementIndex);
            wc_Sha256Update(&sha256, data.data(), data.size());
            ElementType hash[WC_SHA256_DIGEST_SIZE] = {0};
            wc_Sha256Final(&sha256, hash);
            ElementType hashString[WC_SHA256_DIGEST_SIZE * 2] = {0};
            for (std::size_t i{}; i < WC_SHA256_DIGEST_SIZE; ++i) {
                auto ptr = reinterpret_cast<char *>(hashString + i * 2);
                std::sprintf(ptr, "%02x", hash[i]);
            }
            transmit(hashString);
            transmit("\r\n");
        }
        receive_buffer_guard = false;
    }
}

} // namespace shell

extern "C" {

void shell_run() { shell::run(); }

void shell_receive_callback(unsigned char *const data, const uint32_t size) {
    using namespace shell;
    if (receive_buffer_guard) {
        return;
    }
    receive_buffer.Append({data, size});
    receive_buffer_guard = true;
}
}
