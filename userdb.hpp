#include <array>
#include <cstdint>
#include <span>
#include <string>

constexpr size_t HASH_LENGTH = 32;
constexpr size_t SAULT_LENGTH = 32;
constexpr size_t LOGIN_LENGTH = 20; // login and password length
constexpr size_t MAX_USERS = 10;
constexpr size_t MAX_MESSAGE_LENGTH = 256; // reserve for `Users` Action (for others 32 is enough)
constexpr uint32_t USERS_FLASH_ADDRESS = 0x08060000;

using BytesSpan = std::span<const uint8_t>;

enum class ActionType : uint8_t {
    Auth = 0x00,
    Users,
    ChangePassword,
    AddUser,
    DelUser,
    Unknown,
};

struct __attribute__((packed, aligned(4))) UserRecord {
    std::array<uint8_t, LOGIN_LENGTH> login{};
    std::array<uint8_t, HASH_LENGTH> hash{};
    std::array<uint8_t, SAULT_LENGTH> sault{};
    std::string toString();
    UserRecord() = default;
    UserRecord(const std::array<uint8_t, LOGIN_LENGTH> &, const std::array<uint8_t, HASH_LENGTH> &h,
               const std::array<uint8_t, SAULT_LENGTH> &);
};

class UserDb {
public:
    static UserDb &instance();
    std::string doAction();
    ActionType getActionType() const;
    std::string setAction(BytesSpan);
    bool IsAdminSet();

private:
    UserDb();
    UserDb(const UserDb &) = delete;
    UserDb &operator=(const UserDb &) = delete;

    // FIELDS
    std::array<UserRecord, MAX_USERS> userRecords;
    ActionType actionType;
    BytesSpan data;
    uint32_t userCount;
    bool isAdminSet;
    // TODO: replace `std::string`s with this buffer
    // std::array<char, MAX_MESSAGE_LENGTH> message_buffer{};

    // USER ACTIONS
    std::string Auth(const BytesSpan login, const BytesSpan password) const;
    std::string Users() const;
    std::string ChangePassword(BytesSpan, BytesSpan, BytesSpan);
    std::string AddUser(BytesSpan, BytesSpan, BytesSpan);
    std::string DelUser(BytesSpan, BytesSpan);

    // INTERNAL FUNCTIONS
    uint8_t writeUser();
    void readUser(UserRecord &);
    std::string ReadDb();
    std::string AddDefaultAdmin();
    uint8_t FindUser(const BytesSpan) const;
    void generateRandomBlock(uint8_t *, size_t);
};
