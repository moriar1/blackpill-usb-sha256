#include <cstdint>
#include <iostream>
#include <span>
#include <string>

enum class ActionType : std::uint8_t {
    Auth = 0x00,
    Users,
    ChangePassword,
    AddUser,
    DelUser,
};

class Actions {
public:
    Actions(std::span<std::uint8_t> d) : data(d) {
        if (*data.begin() > 0x04) {
            // err
        }
        actionType = static_cast<ActionType>(data.front());
    }

    std::string getActionString() const {
        switch (actionType) {
        case ActionType::Auth: // 0x00
            return std::string("Authentification");
        case ActionType::Users: // 0x01
            return std::string("Users");
        case ActionType::ChangePassword: // 0x02
            return std::string("ChangePassword");
        case ActionType::AddUser: // 0x03
            return std::string("AddUser");
        case ActionType::DelUser: // 0x04
            return std::string("DelUser");
        }
        return std::string("Unknown");
    }

    std::string doAction() {
        // TODO std::move()?
        std::span<std::uint8_t> firstArg = data.subspan(1, 8);
        std::span<std::uint8_t> secondArg = data.subspan(8, 16);
        std::span<std::uint8_t> thirdArg = data.subspan(16, 24);

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

private:
    ActionType actionType;
    std::span<std::uint8_t> data;

    std::string Auth(const std::span<const std::uint8_t> login,
                     const std::span<const std::uint8_t> password) const {
        return std::string("Ok");
    }

    std::string Users() const { return ("Ok"); }

    // TODO: add const
    std::string ChangePassword(std::span<std::uint8_t> login, std::span<std::uint8_t> oldPassword,
                               std::span<std::uint8_t> newPassword) {
        return std::string("Ok");
    }

    std::string AddUser(std::span<std::uint8_t> password, std::span<std::uint8_t> userLogin,
                        std::span<std::uint8_t> userPassword) {
        return std::string("Ok");
    }

    std::string DelUser(std::span<std::uint8_t> password, std::span<std::uint8_t> userLogin) {
        return std::string("Ok");
    }
};

// switch (actionType) {
// case ActionType::Auth: // 0x00
//   Auth(data);
//   return std::string("Authentification");
// case ActionType::Users: // 0x01
//   Users();
//   break;
// case ActionType::ChangePassword: // 0x02
//   ChangePassword(data);
//   break;
// case ActionType::AddUser: // 0x03
//   AddUser(data);
//   break;
// case ActionType::DelUser: // 0x04
//   DelUser(data);
//   break;
// }

int main() {
    // if (isFirstTime()) {
    //   setAdmin();
    // }

    // TODO: add const
    std::uint8_t arr[] = {1, 2, 3, 4, 5, '\r'};
    std::span<std::uint8_t> data(arr);

    if (*data.rbegin() != '\r') {
        data = data.first(data.size() - 1);

        if (data.size() < 3 || data.size() > 2000) {
            std::cout << "Err" << std::flush;
        }

        const Actions userAction{std::move(data)};
        std::string actionMessage = userAction.getActionString() +
                                    " (code: " + std::to_string(static_cast<int>(data.front())) +
                                    ")";

        if (actionMessage == "Unknown") {
            std::cout << actionMessage;
            // exit
        }

        actionMessage += " is being performed...";
        // usb.Transmit(actionMessage);
        std::cout << actionMessage;

        for (const auto &num : data) {
            std::cout << static_cast<int>(num) << " " << std::flush;
        }
        // userAction.doAction();
    }
}
