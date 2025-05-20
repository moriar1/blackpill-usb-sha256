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

            // clang-format off
            // TESTS BEGIN
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

            // // OLD Test. Command = 0x0, 1st argument = `username0`, 2nd = `password1`, 3rd = `myarg2`
            // // std::array<uint8_t, 64> a{
            // //    0x1, 'u', 's', 'e', 'r', 'n', 'a', 'm', 'e', '0', '0', '0', '0', '0', '0', '0',
            // //    '0', '0', '0', '0', '0', 'z', 'p', 'a', 's', 's', 'w', 'o', 'r', 'd', '1', '1',
            // //    '1', '1', '1', '1', '1', '1', '1', '1', '1', '1', 'z', 'm', 'y', 'a', 'r', 'g',
            // //    'u', 'm', 'e', 'n', 't', '2', '2', '2', '2', '2', '2', '2', '2', '2', '2', 'z',
            // // };
            // // data = BytesSpan(a);

            // TEST 1 ADDUSER
            // std::array<uint8_t, 64> user1 = {
            // 0x03,
            // 'q','w','e','r','t','y',0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
            // 'z','x','c','v',0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
            // 'm','y','p','a','s','s',0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
            // };
            // data = BytesSpan(user1);
            // // TEST 1
             
            // actionMessage = "Action code: " + std::to_string(static_cast<int>(data.front()));
            // usb.Transmit(actionMessage);
            // usb.Transmit("\r\nArgs: ");
            // usb.Transmit(data.subspan(1, data.size() - 1));
            // usb.Transmit("\r\n");

            // // Execute command
            // userDb.setAction(data);
            // res = userDb.doAction();
            // usb.Transmit(res);
            // usb.Transmit("\r\n");
            // usb.ClearBuffer();
            
            // // TEST 2 USERS
            // std::array<uint8_t, 64> users_cmd = {0x01};
            // data = BytesSpan(users_cmd);
            // // TEST 2

            // actionMessage = "Action code: " + std::to_string(static_cast<int>(data.front()));
            // usb.Transmit(actionMessage);
            // usb.Transmit("\r\nArgs: ");
            // usb.Transmit(data.subspan(1, data.size() - 1));
            // usb.Transmit("\r\n");

            // // Execute command
            // userDb.setAction(data);
            // res = userDb.doAction();
            // usb.Transmit(res);
            // usb.Transmit("\r\n");
            // usb.ClearBuffer();
            
            // // TEST 3 AUTH
            // std::array<uint8_t, 64> auth_cmd = {
            // 0x00,
            // 'z','x','c','v',0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
            // 'm','y','p','a','s','s',0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
            // };
            // data = BytesSpan(auth_cmd);
            // // TEST 3

            // TESTS END
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

            // clang-format on
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
