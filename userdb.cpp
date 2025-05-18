#include "board/main.h"
#include "userdb.hpp"
#include <wolfssl/ssl.h>
#include <wolfssl/wolfcrypt/pwdbased.h>

UserRecord::UserRecord(const std::array<uint8_t, LOGIN_LENGTH> &l,
                       const std::array<uint8_t, HASH_LENGTH> &h,
                       const std::array<uint8_t, SAULT_LENGTH> &s)
    : login(l), hash(h), sault(s) {}

std::string UserRecord::toString() {
    std::string out("Login: ");
    for (auto &n : login) {
        out += n;
    }
    out += "\r\nHash: ";
    for (auto &n : hash) {
        out += std::to_string(n) + ' ';
    }
    out += "\r\nSault: ";
    for (auto &n : sault) {
        out += std::to_string(n) + ' ';
    }
    out += "\r\n";
    return out;
}

UserDb &UserDb::instance() {
    static UserDb instance;
    return instance;
}

std::string UserDb::setAction(BytesSpan d) {
    // TODO:
    // - data size !=64
    // - control bytes (21st byte in every arg) != 0
    data = d;
    if (*data.begin() > 0x04) {
        // Err
    }
    actionType = static_cast<ActionType>(data.front());
    return ("Ok");
}

std::string UserDb::DelUser(BytesSpan adminPassword, BytesSpan userLogin) {
    if (userLogin.back() || adminPassword.back()) {
    }
    return std::string("Ok");
}

void UserDb::generateRandomBlock(uint8_t *buffer, size_t length) {
    uint32_t seed = HAL_GetUIDw0() ^ HAL_GetUIDw1() ^ HAL_GetUIDw2() ^ HAL_GetTick();
    for (size_t i = 0; i < length; i++) {
        buffer[i] = (uint8_t)((seed >> (8 * (i % 4))) & 0xFF);
        seed = seed * 1664525 + 1013904223;
    }
}

std::string UserDb::AddUser(BytesSpan adminPassword, BytesSpan userLogin, BytesSpan userPassword) {
    if (adminPassword.back()) {
    }

    byte userSault[SAULT_LENGTH];
    generateRandomBlock(userSault, SAULT_LENGTH);

    byte userHash[HASH_LENGTH];
    int ret = wc_PBKDF2(userHash, userPassword.data(), userPassword.size(), userSault, SAULT_LENGTH,
                        2048, HASH_LENGTH, WC_SHA256);
    if (ret != 0) {
        return "wc_PBKDF2 failed";
    }

    std::array<uint8_t, LOGIN_LENGTH> login{};
    std::array<uint8_t, HASH_LENGTH> hash{};
    std::array<uint8_t, SAULT_LENGTH> sault{};

    std::copy(userLogin.begin(), userLogin.begin() + LOGIN_LENGTH, login.begin());

    std::copy(std::begin(userHash), std::begin(userHash) + HASH_LENGTH, hash.begin());
    std::copy(std::begin(userSault), std::begin(userSault) + SAULT_LENGTH, sault.begin());

    UserRecord user{login, hash, sault};
    userRecords[userCount++] = std::move(user);

    if (writeUser(user) != HAL_OK) {
        return "writing user into flash memory failed";
    }
    return "Ok";
}

std::string UserDb::ChangePassword(BytesSpan login, BytesSpan oldPassword, BytesSpan newPassword) {
    if (login.back() || oldPassword.back() || newPassword.back()) {
    }
    return std::string("Ok");
}

std::string UserDb::Users() const {
    std::string output{};
    for (uint8_t i = 0; i < userCount; i++) {
        for (auto &n : userRecords[i].login) {
            output += n;
        }
    }
    return output == std::string{} ? "No users found" : output;
}

uint8_t UserDb::FindUser(const BytesSpan login) const {
    for (uint8_t idx = 0; idx < userCount; idx++) {
        // Bytewise comparing
        for (uint8_t j = 0; j < LOGIN_LENGTH; j++) {
            if (userRecords[idx].login[j] != login[j]) {
                continue;
            }
        }
        return idx;
    }
    return UINT8_MAX;
}

std::string UserDb::Auth(const BytesSpan login, const BytesSpan password) const {
    uint8_t idx = FindUser(login);
    if (idx >= userCount) {
        return "no user found";
    }
    byte userHash[HASH_LENGTH];

    // Get hash using password and sault
    int ret = wc_PBKDF2(userHash, password.data(), password.size(), userRecords[idx].sault.data(),
                        SAULT_LENGTH, 2048, HASH_LENGTH, WC_SHA256);
    if (ret != 0) {
        return "getting hash failed";
    }

    // Compare hashes (strcmp do not works)
    for (uint8_t i = 0; i < HASH_LENGTH; i++) {
        if (userHash[i] != userRecords[idx].hash[i]) {
            return "password is incorrect";
        }
    }
    return std::string("Ok");
}

std::string UserDb::doAction() {
    constexpr size_t ARG_INPUT_SIZE = 21; // In package
    constexpr size_t ARG_USED_SIZE = 20;  // In using
    // TODO: Check 21st byte == 0x0

    BytesSpan firstArg = data.subspan(1, ARG_USED_SIZE);                      // 1..20
    BytesSpan secondArg = data.subspan(1 + ARG_INPUT_SIZE, ARG_USED_SIZE);    // 22..41
    BytesSpan thirdArg = data.subspan(1 + 2 * ARG_INPUT_SIZE, ARG_USED_SIZE); // 43..62

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
    default:
        return std::string("Unknown");
    }
    return std::string("Unknown");
}

// TODO: Defalut initilize other fields
UserDb::UserDb() : userRecords{}, actionType(ActionType::Unknown), data{} {
    // Check if db is empty
    if (*((__IO uint32_t *)USERS_FLASH_ADDRESS) != 0xffffffff) {
        userCount = 0; // `userCount` reads from flash in ReadDb()
        ReadDb();
        isAdminSet = true;
    } else {
        AddDefaultAdmin();
        isAdminSet = false;
        userCount = 1;
    }
}

uint8_t UserDb::writeUser(UserRecord &userRecord) {
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
        status =
            HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, USERS_FLASH_ADDRESS + (i * 4), dataPtr[i]);
        if (status != HAL_OK) {
            break;
        }
    }

    HAL_FLASH_Lock();
    __enable_irq();

    return status;
}

void UserDb::readUser(UserRecord &userRecord) {
    uint32_t *flashPtr = reinterpret_cast<uint32_t *>(USERS_FLASH_ADDRESS);
    uint32_t *structPtr = (uint32_t *)&userRecord;

    for (uint32_t i = 0; i < sizeof(userRecord) / 4; i++) {
        structPtr[i] = flashPtr[i];
    }
}

ActionType UserDb::getActionType() const { return actionType; }

std::string UserDb::AddDefaultAdmin() { return "Ok"; }

std::string UserDb::ReadDb() {
    userCount = *((__IO uint32_t *)(USERS_FLASH_ADDRESS + sizeof(uint32_t)));
    return "Ok";
};

bool UserDb::IsAdminSet() { return true; }
