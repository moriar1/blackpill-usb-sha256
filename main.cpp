#include <array>
#include <cstdint>
#include <span>
#include <string>
#include <wolfssl/ssl.h>
#include <wolfssl/wolfcrypt/pwdbased.h>

#include "board/main.h"
#include "board/stm32f4xx_hal_conf.h"
#include "board/stm32f4xx_it.h"
#include "usb-sha256/hex_string.hpp"
#include "usb-sha256/sha256sum.hpp"
#include "usb-sha256/types.hpp"
#include "usb-sha256/usb.hpp"

using BytesSpan = std::span<const std::uint8_t>;

enum class ActionType : std::uint8_t {
    Auth = 0x00,
    Users,
    ChangePassword,
    AddUser,
    DelUser,
};

constexpr uint8_t HASH_LENGTH = 32;
constexpr uint8_t SAULT_LENGTH = 32;
constexpr uint8_t LOGIN_LENGTH = 20; // and password
constexpr uint8_t MAX_USERS = 2;

struct __attribute__((packed, aligned(4))) UserRecord {
    std::array<uint8_t, LOGIN_LENGTH> login;
    std::array<uint8_t, HASH_LENGTH> hash;
    std::array<uint8_t, SAULT_LENGTH> sault;

    // TODO: replace with static buffer
    std::string toString() {
        std::string out("Login: "); // or use stringstream
        for (auto &n : login) {
            out += n;
        }
        out += "\r\nHash: ";
        for (auto &n : hash) {
            out += std::to_string(n);
            out += ' ';
            // out += n;
        }
        out += "\r\nSault: ";
        for (auto &n : sault) {
            out += std::to_string(n);
            out += ' ';
            // out += n;
        }
        out += "\r\n";

        return out;
    }
};

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
/////////////////////////////////////////////////////////////////////
class UserDb {
public:
    UserDb();
    // std::string getActionString() const;
    // char *getActionString() const;
    std::string doAction();
    ActionType getActionType() const;
    std::string setAction(BytesSpan);
    bool IsAdminSet();
    std::array<UserRecord, MAX_USERS> userRecords; // <--

private:
    // var
    ActionType actionType;
    BytesSpan data;
    uint8_t userCount{};
    bool isAdminSet;

    std::string Auth(const BytesSpan login, const BytesSpan password) const;
    std::string Users() const;
    std::string ChangePassword(BytesSpan, BytesSpan, BytesSpan);
    std::string AddUser(BytesSpan, BytesSpan, BytesSpan);
    std::string DelUser(BytesSpan, BytesSpan);

    uint32_t Read4Bytes();
    std::string ReadDb();
    std::string AddDefaultAdmin();
};

std::string UserDb::AddDefaultAdmin() { return "Ok"; }

std::string UserDb::ReadDb() { return "Ok"; };
bool UserDb::IsAdminSet() { return true; }

std::string UserDb::setAction(BytesSpan d) {
    data = d;
    if (*data.begin() > 0x04) {
        // err
    }
    actionType = static_cast<ActionType>(data.front());
    return ("Ok");
}

std::string UserDb::DelUser(BytesSpan, BytesSpan userLogin) { return std::string("Ok"); }

std::string UserDb::AddUser(BytesSpan adminPassword, BytesSpan userLogin, BytesSpan userPassword) {
    // TODO:
    // - check lengths
    // - check username availability
    // - verificate adminPassword

    // Generate sault
    RNG rng;
    byte userSault[SAULT_LENGTH];

    int ret = wc_InitRng(&rng);
    if (ret != 0) {
        return "init of rng failed";
    }

    ret = wc_RNG_GenerateBlock(&rng, userSault, SAULT_LENGTH);
    if (ret != 0) {
        return "generating rng block failed";
    }
    wc_FreeRng(&rng);

    // Get hash
    byte userHash[HASH_LENGTH];
    ret = wc_PBKDF2(userHash, userPassword.data(), userPassword.size(), userSault, SAULT_LENGTH,
                    2048, HASH_LENGTH, WC_SHA256);
    if (ret != 0) {
        return "wc_PBKDF2 failed";
    }

    std::array<uint8_t, LOGIN_LENGTH> login{};
    std::array<uint8_t, HASH_LENGTH> hash{};
    std::array<uint8_t, SAULT_LENGTH> sault{};

    std::copy(userLogin.begin(), userLogin.begin() + LOGIN_LENGTH, login.begin());

    std::copy(std::begin(userHash), std::end(hash), hash.begin());
    std::copy(std::begin(userSault), std::end(sault), sault.begin());

    UserRecord user{login, hash, sault};
    userRecords[userCount++] = std::move(user);

    if (writeUser(user) != HAL_OK) {
        return "writing user into flash memory failed";
    }
    return "Ok";
}

std::string UserDb::ChangePassword(BytesSpan login, BytesSpan oldPassword, BytesSpan newPassword) {
    return std::string("Ok");
}

std::string UserDb::Users() const { return std::string("Ok"); }

std::string UserDb::Auth(const BytesSpan login, const BytesSpan password) const {
    return std::string("Ok");
}

ActionType UserDb::getActionType() const { return actionType; }

std::string UserDb::doAction() {

    std::array<uint8_t, 64> a{
        'a', 'b', 'c', 'b', 'c', 'b', 'c', 'b', 'c', 'b', 'c', 'b', 'c', 'b', 'c', 'b',
        'c', 'b', 'c', 'b', 'c', 'b', 'c', 'b', 'c', 'b', 'c', 'b', 'c', 'b', 'c', 'b',
        'c', 'b', 'c', 'b', 'c', 'b', 'c', 'b', 'c', 'b', 'c', 'b', 'c', 'b', 'c', 'b',
        'c', 'b', 'c', 'b', 'c', 'b', 'c', 'b', 'c', 'b', 'c', 'b', 'c', 'b', 'c', 'b',
    };
    data = BytesSpan(a);

    constexpr size_t sz = 21;                   // TODO: add last zero to check data correctness
    BytesSpan firstArg = data.subspan(1, 20);   // 1..21 (21st byte is 0)
    BytesSpan secondArg = data.subspan(22, 20); // 22..42 (42nd byte is 0)
    BytesSpan thirdArg = data.subspan(43, 20);  // 43..63 (63rd byte is 0)

    switch (actionType) {
    case ActionType::Auth: // 0x00
        return Auth(firstArg, secondArg);
    case ActionType::Users: // 0x01
        return Users();
    case ActionType::ChangePassword: // 0x02
        return ChangePassword(firstArg, secondArg, thirdArg);
    case ActionType::AddUser: // 0x03
        return AddUser(firstArg, secondArg, thirdArg);
    case ActionType::DelUser: // 0x04
        return DelUser(firstArg, secondArg);
    }
    return std::string("Unknown");
}

// TODO: defalut initilize other fields
UserDb::UserDb() {
    if (Read4Bytes() != 0xffffffff) {
        ReadDb();
        isAdminSet = true;
    } else {
        AddDefaultAdmin();
        isAdminSet = false;
    }
    actionType = ActionType::AddUser;
}

uint32_t UserDb::Read4Bytes() { return 0x0; }

int main() {
    board_main();
    wolfSSL_Init();

    using namespace usbsha256;
    Usb &usb = Usb::instance();

    UserDb userDb{};

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

                // if (userDb.getActionType() == ActionType::ChangePassword) {
                if (*data.rbegin() == '1') {
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

    // ---- Main loop ---- //
    while (true) {
        usb.WaitReceiving();
        auto data = usb.GetBuffer();
        if (!data.empty() && (data.back() == '\r' || data.back() == '\n')) {
            data = data.first(data.size() - 1);
            std::string actionMessage =
                "Action code: " + std::to_string(static_cast<int>(data.front()));
            // userDb.setAction(std::move(data));

            // For debug
            usb.Transmit(actionMessage);
            usb.Transmit("\r\nArgs: ");
            usb.Transmit(data.subspan(1, data.size()));
            usb.Transmit("\r\n");

            // Execute command
            usb.Transmit("proceeding...\r\n");
            std::string res = userDb.doAction();
            if (res == "Ok") {
                usb.Transmit(userDb.userRecords[0].toString());
                usb.Transmit("\r\n");
                usb.ClearBuffer();
            }
            usb.Transmit(res);
            usb.Transmit("\r\n");
            usb.ClearBuffer();

            /// ADD USER ///
            // UserRecord user1{};
            // user1.login = {'a', 'b', 'c'};
            // user1.hash = {'h', 'a', 's'};
            // user1.sault = {'s', 's', 's'};

            // if (writeUser(user1) != HAL_OK) {
            //     usb.Transmit("Flash write error\r\n");
            //     usb.ClearBuffer();
            // }

            // user1.login = {};
            // user1.hash = {};
            // user1.sault = {};

            // readUser(user1);

            // usb.Transmit(user1.toString());
            // usb.ClearBuffer();
            /// ADD USER ///
        }
    }
}
