#include <algorithm>
#include <array>
#include <atomic>
#include <cstddef>
#include <ranges>
#include <span>
#include <string_view>

#include "shell.h"
#include "static_buffer.hpp"
#include "usbd_cdc_if.h"

namespace shell {

using RxBuffer = static_buffer::Buffer<APP_RX_DATA_SIZE>;
using TxBuffer = static_buffer::Buffer<APP_TX_DATA_SIZE>;
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
    auto ptr = reinterpret_cast<const static_buffer::ElementType *>(text.data());
    SpanType data{ptr, text.size()};
    transmit(data);
}

void run() {
    while (true) {
        while (!receive_buffer_guard) {
        }
        auto span = receive_buffer.Data();
        if (span[0] == '\r') {
            transmit("hello\r\n");
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
    receive_buffer.Assign({data, size});
    receive_buffer_guard = true;
}
}
