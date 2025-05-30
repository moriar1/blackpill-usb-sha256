#include "usb-sha256/usb.hpp"
#include "userdb.hpp"
#include <board/main.h>
#include <span>
#include <string>
#include <wolfssl/ssl.h>

int main() {
    board_main();
    wolfSSL_Init();
    usbsha256::Usb &usb = usbsha256::Usb::instance();
    UserDb &userDb = UserDb::instance();
    std::string actionMessage, res;

    // If there is no users in flash-memory
    // Ask user to change admin's password
    if (!userDb.IsAdminSet()) {
        while (true) {
            usb.WaitReceiving();
            auto data = usb.GetBuffer();
            if (!data.empty() && (data.back() == '\r' || data.back() == '\n')) {
                data = data.first(data.size() - 1);

                userDb.setAction(data);
                if (userDb.getActionType() == ActionType::ChangePassword) {
                    usb.Transmit("Changing admin's password...\r\n");
                    res = userDb.doAction();
                    if (res == "Ok") {
                        usb.Transmit("Password changed.");
                        usb.ClearBuffer();
                        break;
                    } else {
                        usb.Transmit(res);
                        usb.Transmit("\r\n");
                        usb.ClearBuffer();
                        continue;
                    }
                } else {
                    usb.Transmit("You must `changepassword` for `admin`\r\n");
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
