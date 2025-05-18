#include "usb-sha256/usb.hpp"
#include "userdb.hpp"
#include <array>
#include <board/main.h>
#include <cstdint>
#include <span>
#include <string>
#include <wolfssl/ssl.h>

int main() {
    board_main();
    wolfSSL_Init();
    usbsha256::Usb &usb = usbsha256::Usb::instance();
    UserDb &userDb = UserDb::instance();

    // If there is no users in flash-memory
    // Ask user to change admin's password
    if (!userDb.IsAdminSet()) {
        while (true) {
            usb.WaitReceiving();
            auto data = usb.GetBuffer();
            if (!data.empty() && (data.back() == '\r' || data.back() == '\n')) {
                data = data.first(data.size() - 1);

                std::string actionMessage =
                    "Action code: " + std::to_string(static_cast<int>(data.front()));

                userDb.setAction(std::move(data));
                usb.Transmit(actionMessage);
                usb.Transmit("\r\n");

                if (userDb.getActionType() == ActionType::ChangePassword) {
                    usb.Transmit("Changing password...\r\n");
                    usb.Transmit(userDb.doAction());
                    usb.Transmit("\r\n");
                    usb.ClearBuffer();
                    // TODO: check if no errors then break
                    break;
                } else {
                    usb.Transmit("You must `changepassword` for `admin` before proceeding\r\n");
                    usb.ClearBuffer();
                }
            }
        }
    }

    // Main loop
    while (true) {
        usb.WaitReceiving();
        auto data = usb.GetBuffer();
        if (!data.empty() && (data.back() == '\r' || data.back() == '\n')) {
            data = data.first(data.size() - 1);

            // Test. Command = 0x0, 1st argument = `username0`, 2nd = `password1`, 3rd = `myarg2`
            std::array<uint8_t, 64> a{
                0x3, 'u', 's', 'e', 'r', 'n', 'a', 'm', 'e', '0', '0', '0', '0', '0', '0', '0',
                '0', '0', '0', '0', '0', 'z', 'p', 'a', 's', 's', 'w', 'o', 'r', 'd', '1', '1',
                '1', '1', '1', '1', '1', '1', '1', '1', '1', '1', 'z', 'm', 'y', 'a', 'r', 'g',
                'u', 'm', 'e', 'n', 't', '2', '2', '2', '2', '2', '2', '2', '2', '2', '2', 'z',
            };
            data = BytesSpan(a);
            std::string actionMessage =
                "Action code: " + std::to_string(static_cast<int>(data.front()));
            usb.Transmit(actionMessage);
            usb.Transmit("\r\nArgs: ");
            usb.Transmit(data.subspan(1, data.size() - 1));
            usb.Transmit("\r\n");

            // Execute command
            userDb.setAction(std::move(data));
            std::string res = userDb.doAction();
            usb.Transmit(res);
            usb.Transmit("\r\n");
            usb.ClearBuffer();
        }
    }
}
