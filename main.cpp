#include <wolfssl/ssl.h>

#include "board/main.h"
#include "usb-sha256/usb.hpp"
#include "usb-sha256/sha256sum.hpp"
#include "usb-sha256/hex_string.hpp"

int main() {
    board_main();
    wolfSSL_Init();

    using namespace usbsha256;
    Usb& usb = Usb::instance();

    while (true) {
        usb.WaitReceiving();
        auto data = usb.GetBuffer();
        if (*data.rbegin() == '\r') {
            data = data.first(data.size() - 1);

            Sha256Sum sha256{};
            const auto hash = sha256.Hash(data);
            const HexString<WC_SHA256_DIGEST_SIZE> hex{hash};

            usb.Transmit(hex.String());
            usb.Transmit("\r\n");
            usb.ClearBuffer();
        }
    }
}
