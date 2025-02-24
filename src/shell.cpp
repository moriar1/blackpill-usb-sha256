#include <algorithm>
#include <array>
#include <string_view>

#include "shell.h"
#include "usbd_cdc_if.h"

namespace shell {

std::string_view receivedData{};

void transmit(std::string_view data) {
    static std::array<unsigned char, APP_TX_DATA_SIZE> buffer{};
    if (data.size() > buffer.size()) {
        return;
    }
    std::copy(data.cbegin(), data.cend(), buffer.begin());
    while (CDC_Transmit_FS(buffer.data(), data.size()) == USBD_BUSY) {
    }
}

void run() {
    while (true) {
        while (receivedData.empty()) {
        }
        if (receivedData[0] == '\r') {
            transmit("hello\r\n");
        }
        receivedData = {};
    }
}

} // namespace shell

extern "C" {

void shell_run() { shell::run(); }

void shell_receive_callback(unsigned char *const data, const uint32_t size) {
    shell::receivedData = {reinterpret_cast<const char *>(data), size};
}
}
