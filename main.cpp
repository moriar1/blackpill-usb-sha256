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

                userDb.setAction(data);
                usb.Transmit(actionMessage);
                usb.Transmit("\r\nArgs: ");
                usb.Transmit(data.subspan(1, data.size() - 1));
                usb.Transmit("\r\n");

                if (userDb.getActionType() == ActionType::ChangePassword) {
                    usb.Transmit("Changing admin's password...\r\n");
                    std::string res = userDb.doAction();
                    usb.Transmit(res);
                    usb.Transmit("\r\n");
                    usb.ClearBuffer();
                    if (res != "Ok") {
                        // usb.Transmit("Err");
                        usb.Transmit(res);
                        usb.Transmit("\r\n");
                        usb.ClearBuffer();
                        continue;
                    }
                    break;
                } else {
                    usb.Transmit("You must `changepassword` for `admin`\r\n");
                    usb.ClearBuffer();
                }
            }
        }
    }

    std::string actionMessage, res;

    // Main loop
    while (true) {
        usb.WaitReceiving();
        auto data = usb.GetBuffer();
        if (!data.empty() && (data.back() == '\r' || data.back() == '\n')) {
            data = data.first(data.size() - 1);

            actionMessage = "Action code: " + std::to_string(static_cast<int>(data.front()));
            usb.Transmit(actionMessage);
            usb.Transmit("\r\nArgs: ");
            usb.Transmit(data.subspan(1, data.size() - 1));
            usb.Transmit("\r\n");

            // Execute command
            res = userDb.setAction(data);
            if (res != "Ok") {
                usb.Transmit(res);
                usb.Transmit("\r\n");
                usb.ClearBuffer();
                continue;
            }
            res = userDb.doAction();
            usb.Transmit(res);
            usb.Transmit("\r\n");
            usb.ClearBuffer();
        }
    }
}
