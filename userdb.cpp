#include "board/main.h"
#include "userdb.hpp"
#include <cstdint>
#include <string>
#include <wolfssl/ssl.h>
#include <wolfssl/wolfcrypt/pwdbased.h>

UserRecord::UserRecord(const std::array<uint8_t, LOGIN_LENGTH> &l,
                       const std::array<uint8_t, HASH_LENGTH> &h,
                       const std::array<uint8_t, SAULT_LENGTH> &s)
    : login(l), hash(h), sault(s) {}

UserDb &UserDb::instance() {
    static UserDb instance;
    return instance;
}

std::string UserDb::setAction(BytesSpan d) {
    data = d;
    if (data.front() > 0x04) {
        return "Unknown action";
    }
    actionType = static_cast<ActionType>(data.front());
    return "Ok";
}

std::string UserDb::DelUser(BytesSpan adminPassword, BytesSpan userLogin) {
    uint8_t userIdx = FindUser(userLogin);
    if (userIdx >= userCount) {
        return "User not found";
    }
    if (userIdx == 0) {
        return "Can't delete admin";
    }
    BytesSpan adminLogin{userRecords[0].login.data(), userRecords[0].login.size()};
    std::string authResult = Auth(adminLogin, adminPassword);
    if (authResult != "Ok") {
        return "admin password incorrect";
    }
    // shift users
    for (uint8_t i = userIdx; i < userCount - 1; i++) {
        userRecords[i] = std::move(userRecords[i + 1]);
    }
    userCount--;
    if (writeUser() != HAL_OK) {
        return "writing to flash memory failed";
    }

    return "Ok";
}

void UserDb::generateRandomBlock(uint8_t *buffer, size_t length) {
    uint32_t seed = HAL_GetUIDw0() ^ HAL_GetUIDw1() ^ HAL_GetUIDw2() ^ HAL_GetTick();
    for (size_t i = 0; i < length; i++) {
        buffer[i] = (uint8_t)((seed >> (8 * (i % 4))) & 0xFF);
        seed = seed * 1664525 + 1013904223;
    }
}

std::string UserDb::AddUser(BytesSpan adminPassword, BytesSpan userLogin, BytesSpan userPassword) {
    // Checks
    if (FindUser(userLogin) != UINT8_MAX) {
        return "user already exists";
    }
    if (userCount >= MAX_USERS) {
        return "maximum user count is reached";
    }
    BytesSpan adminLogin{userRecords[0].login.data(), userRecords[0].login.size()};
    std::string authResult = Auth(adminLogin, adminPassword);
    if (authResult != "Ok") {
        return "admin password incorrect";
    }

    // Sault and hash generation
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

    if (writeUser() != HAL_OK) {
        userCount--;
        return "writing user into flash memory failed";
    }
    return "Ok";
}

std::string UserDb::ChangePassword(BytesSpan login, BytesSpan oldPassword, BytesSpan newPassword) {
    uint8_t idx = FindUser(login);
    if (idx >= userCount) {
        return "User not found";
    }
    std::string res = Auth(login, oldPassword);
    if (res != "Ok") {
        return "Old password is incorrect: " + res;
    }

    byte newSault[SAULT_LENGTH];
    generateRandomBlock(newSault, SAULT_LENGTH);
    byte newHash[HASH_LENGTH];
    int ret = wc_PBKDF2(newHash, newPassword.data(), newPassword.size(), newSault, SAULT_LENGTH,
                        2048, HASH_LENGTH, WC_SHA256);
    if (ret != 0) {
        return "hash generation failed";
    }
    std::copy(std::begin(newHash), std::begin(newHash) + HASH_LENGTH,
              userRecords[idx].hash.begin());
    std::copy(std::begin(newSault), std::begin(newSault) + SAULT_LENGTH,
              userRecords[idx].sault.begin());
    if (writeUser() != HAL_OK) {
        return "writing user into flash failed";
    }
    // if someone (including admin) changes password then admin is already set (or is setting now)
    isAdminSet = true;

    return std::string("Ok");
}

std::string UserDb::Users() const {
    std::string output{};
    for (uint8_t i = 0; i < userCount; i++) {
        for (auto &n : userRecords[i].login) {
            // if zero then login ended
            if (n) {
                output += static_cast<char>(n);
            } else {
                break;
            }
        }
        if (i < userCount - 1) {
            output += ", ";
        }
    }
    // There are always admin, but maybe some errors?
    return output.empty() ? "No users found" : output;
}

uint8_t UserDb::FindUser(const BytesSpan login) const {
    for (uint8_t idx = 0; idx < userCount; idx++) {
        bool flag = true;
        // Bytewise comparing
        for (uint8_t j = 0; j < LOGIN_LENGTH; j++) {
            // End of `login` reached
            if (j >= login.size() || login[j] == 0) {
                flag = (userRecords[idx].login[j] == 0);
                break; // Ok
            }
            if (userRecords[idx].login[j] != login[j]) {
                flag = false;
                break;
            }
        }
        if (flag) {
            return idx;
        }
    }
    return UINT8_MAX;
}

std::string UserDb::Auth(const BytesSpan login, const BytesSpan password) const {
    const uint8_t idx = FindUser(login);
    if (idx >= userCount) {
        return "Auth failed";
    }
    byte userHash[HASH_LENGTH];

    // Get hash using password and sault
    const int ret =
        wc_PBKDF2(userHash, password.data(), password.size(), userRecords[idx].sault.data(),
                  SAULT_LENGTH, 2048, HASH_LENGTH, WC_SHA256);
    if (ret != 0) {
        return "Auth failed";
    }

    // Compare hashes
    if (0 != memcmp(userHash, userRecords[idx].hash.data(), HASH_LENGTH)) {
        return "Auth Failed";
    }
    return "Ok";
}

std::string UserDb::doAction() {
    constexpr size_t ARG_INPUT_SIZE = 21; // In package
    constexpr size_t ARG_USED_SIZE = 20;  // In using

    if (data.size() != 3 * ARG_INPUT_SIZE) { // 63 bytes
        return "got invalid data, data size (without \'\\n\'): " + std::to_string(data.size());
    }

    // All `data` bytes are ascii except 0, 21, 42 bytes (and 63 which removes in main)
    // If arg < ARG_USED_SIZE then empty bytes are 0
    // Maybe should check if all bytes are ASCII or zeros
    if (data[21] != 0 || data[42] != 0) {
        return "control bytes with index [21] and [42] do not have 0 values";
    }

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
    return std::string("Unknown"); // extra
}

UserDb::UserDb() : userRecords{}, actionType(ActionType::Unknown), data{} {
    // Check if db is not empty
    if (*((__IO uint32_t *)USERS_FLASH_ADDRESS) != 0xffffffff) {
        userCount = 0; // `userCount` reads from flash in ReadDb()
        const std::string dbOutput = ReadDb();

        if (dbOutput != "Ok") {
            userCount = 0;
            // Still no hadling errors
        }
        isAdminSet = true; // TODO: sets in ReadDb
    } else {
        std::string res = AddDefaultAdmin();
        isAdminSet = false;
        userCount = 1;
    }
}

uint8_t UserDb::writeUser() {
    HAL_StatusTypeDef status;
    FLASH_EraseInitTypeDef FlashErase;
    uint32_t sectorError = 0;

    __disable_irq();
    HAL_FLASH_Unlock();

    // Erase 7th sector
    FlashErase.TypeErase = FLASH_TYPEERASE_SECTORS;
    FlashErase.NbSectors = 1;
    FlashErase.Sector = FLASH_SECTOR_7;
    FlashErase.VoltageRange = FLASH_VOLTAGE_RANGE_3;

    if (HAL_FLASHEx_Erase(&FlashErase, &sectorError) != HAL_OK) {
        HAL_FLASH_Lock();
        __enable_irq();
        return HAL_ERROR;
    }

    // Write `userCount`
    status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, USERS_FLASH_ADDRESS, userCount);
    if (status != HAL_OK) {
        HAL_FLASH_Lock();
        __enable_irq();
        return status;
    }

    // Write `userRecords`
    const uint32_t flashAddr = USERS_FLASH_ADDRESS + sizeof(uint32_t);
    const uint32_t wordsToWrite = sizeof(UserRecord) / 4;
    for (uint32_t userIdx = 0; userIdx < userCount; userIdx++) {
        uint32_t *userPtr = (uint32_t *)&userRecords[userIdx];
        for (uint32_t i = 0; i < wordsToWrite; i++) {
            status =
                HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD,
                                  flashAddr + (userIdx * sizeof(UserRecord)) + (i * 4), userPtr[i]);
            if (status != HAL_OK) {
                HAL_FLASH_Lock();
                __enable_irq();
                return status;
            }
        }
    }
    HAL_FLASH_Lock();
    __enable_irq();
    return status;
}

ActionType UserDb::getActionType() const { return actionType; }

std::string UserDb::AddDefaultAdmin() {
    const std::array<const uint8_t, LOGIN_LENGTH> userLogin = {'a', 'd', 'm', 'i', 'n'};
    const std::array<const uint8_t, LOGIN_LENGTH> userPassword = {'a', 'd', 'm', 'i', 'n'};

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
    userRecords[0] = std::move(user);
    userCount = 1;
    isAdminSet = false;
    if (writeUser() != HAL_OK) {
        return "writing admin into flash failed";
    }
    return "Ok";
}

std::string UserDb::ReadDb() {
    // First 4 bytes are user count
    userCount = *((__IO uint32_t *)USERS_FLASH_ADDRESS);
    if (userCount > MAX_USERS || userCount == 0) {
        return "user count in flash memory is invalid";
    }

    // Next bytes are users
    for (uint32_t userIdx = 0; userIdx < userCount; userIdx++) {
        __IO uint32_t *flashPtr = (__IO uint32_t *)(USERS_FLASH_ADDRESS + sizeof(uint32_t) +
                                                    userIdx * sizeof(UserRecord));

        __IO uint32_t *userPtr = (__IO uint32_t *)&userRecords[userIdx];
        const uint32_t wordsToRead = sizeof(UserRecord) / 4;
        for (uint32_t i = 0; i < wordsToRead; i++) {
            userPtr[i] = flashPtr[i];
        }

        // Check if login if valid
        bool validUser = true;
        for (uint8_t i = 0; i < LOGIN_LENGTH && userRecords[userIdx].login[i] != 0; i++) {
            char c = userRecords[userIdx].login[i];
            if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9'))) {
                validUser = false;
                break;
            }
        }
        if (!validUser)
            return "data is corrupted: found invalid login characters";
    }
    // TODO: Test this checks (now they useless, since `isAdminSet` sets in constructor)
    //
    // Check if first user is admin
    isAdminSet = false;
    std::string adminLogin;
    for (uint8_t i = 0; i < LOGIN_LENGTH && userRecords[0].login[i] != 0; i++) {
        adminLogin += static_cast<char>(userRecords[0].login[i]);
    }
    if (adminLogin != "admin") {
        // return "data is corrupted: first user is not admin";
    }

    // Check if admin's password is not "admin" (hope user will not changepassword to 'admin')
    const std::array<const uint8_t, 5> defPassword = {'a', 'd', 'm', 'i', 'n'};
    uint8_t defHash[HASH_LENGTH];
    const int ret =
        wc_PBKDF2(defHash, defPassword.data(), defPassword.size(), userRecords[0].sault.data(),
                  SAULT_LENGTH, 2048, HASH_LENGTH, WC_SHA256);

    if (!ret && memcmp(defHash, userRecords[0].hash.data(), HASH_LENGTH)) {
        isAdminSet = true;
    }
    return "Ok";
}

bool UserDb::IsAdminSet() { return isAdminSet; }
