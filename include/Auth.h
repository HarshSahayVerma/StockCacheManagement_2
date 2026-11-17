#ifndef AUTH_H
#define AUTH_H

#include <string>
#include <unordered_map>

struct UserRecord
{
    std::string username;
    std::string encryptedPass;
    std::string role;
};

class AuthManager
{
private:
    std::unordered_map<std::string, UserRecord> users;
    std::string userFile;

    static const std::string MASTER_SECRET_KEY;

    static std::string encryptPass(const std::string& plain);
    static std::string decryptPass(const std::string& cipher);

    // NEW: Password validation
    static bool isValidPassword(const std::string& pass);

public:
    explicit AuthManager(const std::string& filename = "users.txt");

    bool loadUsers();
    bool saveUsers() const;

    bool registerUser(
        const std::string& uname,
        const std::string& pass,
        const std::string& role,
        const std::string& secretKey = ""
    );

    bool loginUser(
        const std::string& uname,
        const std::string& pass,
        std::string& outRole
    ) const;

    bool removeUser(
        const std::string& targetUname,
        const std::string& currentAdmin
    );
};

#endif