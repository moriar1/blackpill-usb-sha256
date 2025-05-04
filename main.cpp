#include <cstdint>
#include <span>
#include <string>
#include <wolfssl/ssl.h>

#include "board/main.h"
#include "board/stm32f4xx_hal_conf.h"
#include "board/stm32f4xx_it.h"
#include "usb-sha256/hex_string.hpp"
#include "usb-sha256/sha256sum.hpp"
#include "usb-sha256/types.hpp"
#include "usb-sha256/usb.hpp"

constexpr size_t HASH_LENGTH = 32;
constexpr size_t SAULT_LENGTH = 32;
constexpr size_t LOGIN_LENGTH = 20;

struct __attribute__((packed, aligned(4))) UserRecord {
    std::array<char, LOGIN_LENGTH> login;
    std::array<char, HASH_LENGTH> hash;
    std::array<uint8_t, SAULT_LENGTH> sault;

    std::string toString() {
        std::string out; // or use stringstream
        for (auto &n : login) {
            out += n;
        }
        out += "\r\n";
        for (auto &n : hash) {
            out += std::to_string(n);
            out += ' ';
            // out += n;
        }
        out += "\r\n";
        for (auto &n : sault) {
            out += std::to_string(n);
            out += ' ';
            // out += n;
        }
        out += "\r\n";

        return out;
    }
};

// test_struct test_struct1{};

uint8_t writeUser(UserRecord &userRecord) {
    uint32_t flashAddr = 0x08060000;

    HAL_StatusTypeDef status;
    FLASH_EraseInitTypeDef FlashErase;
    uint32_t sectorError = 0;

    __disable_irq();
    HAL_FLASH_Unlock();

    FlashErase.TypeErase = FLASH_TYPEERASE_SECTORS;
    FlashErase.NbSectors = 1;
    FlashErase.Sector = FLASH_SECTOR_7;
    FlashErase.VoltageRange = FLASH_VOLTAGE_RANGE_3;

    if (HAL_FLASHEx_Erase(&FlashErase, &sectorError) != HAL_OK) {
        HAL_FLASH_Lock();
        __enable_irq();
        return HAL_ERROR;
    }

    uint32_t *dataPtr = (uint32_t *)&userRecord;
    uint32_t wordsToWrite = sizeof(userRecord) / 4;

    for (uint32_t i = 0; i < wordsToWrite; i++) {
        status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, flashAddr + (i * 4), dataPtr[i]);
        if (status != HAL_OK) {
            break;
        }
    }

    HAL_FLASH_Lock();
    __enable_irq();

    return status;
}

void readUser(UserRecord &userRecord) {
    uint32_t flashAddr = 0x08060000; // TODO: glob
    uint32_t *flashPtr = reinterpret_cast<uint32_t *>(flashAddr);
    uint32_t *structPtr = (uint32_t *)&userRecord;

    for (uint32_t i = 0; i < sizeof(userRecord) / 4; i++) {
        structPtr[i] = flashPtr[i];
    }
    // return std::move(struct)
}

constexpr uint32_t MAX_USERS = 128 * 1024 / sizeof(UserRecord);

class UserDb {
    std::array<UserRecord, MAX_USERS> userRecord;
    uint8_t user_count;

public:
    UserDb();
    // void readDb();
    // void addUser();
    // void delUser();
    // bool isAdminSet();
};

UserDb::UserDb() {
    // if (read4Bytes() != 0xffffffff) {
    //     readDb();
    // isAmdinSet = true;
    // } else {
    //     addUser(admin)
    // isAmdinSet = false;
    // }
}

int main() {
    board_main();
    wolfSSL_Init();

    using namespace usbsha256;
    Usb &usb = Usb::instance();

    // UserDb userDb{};
    // if (!userDb.isAdminSet()) {
    //     while (true) {
    // const Actions userAction{std::move(data)};
    // std::string actionMessage = userAction.getActionString() + " (code: " +
    //                             std::to_string(static_cast<int>(data.front())) + ")";

    // if (actionMessage == "Unknown") {
    //     std::cout << actionMessage;
    //     // exit
    // }
    // }
    // }

    // if (!db.isAdminSet()) {
    // *while(true){
    // if action!=changePassToAdmin
    //  "err"
    //   continue
    // else
    //   changePaswd
    //   "ok"
    //   break
    // }
    // }

    while (true) {
        usb.WaitReceiving();
        auto data = usb.GetBuffer();
        if (*data.rbegin() == '\r') {
            data = data.first(data.size() - 1);

            /// ADD USER ///
            UserRecord user1{};
            user1.login = {'a', 'b', 'c'};
            user1.hash = {'h', 'a', 's'};
            user1.sault = {'s', 's', 's'};

            if (writeUser(user1) != HAL_OK) {
                usb.Transmit("Flash write error\r\n");
                usb.ClearBuffer();
            }

            user1.login = {};
            user1.hash = {};
            user1.sault = {};

            readUser(user1);

            usb.Transmit(user1.toString());
            /// ADD USER ///

            usb.Transmit("\r\n");
            usb.ClearBuffer();
        }
    }
}
