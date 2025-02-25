#include <algorithm>
#include <array>
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

void transmit(SpanType data) {
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
        while (receive_buffer.Empty()) {
        }
        auto span = receive_buffer.Data();
        if (span[0] == '\r') {
            transmit("hello\r\n");
        }
        receive_buffer.Clear();
    }
}

} // namespace shell

extern "C" {

void shell_run() { shell::run(); }

void shell_receive_callback(unsigned char *const data, const uint32_t size) {
    shell::receive_buffer.Assign({data, size});
}
}
