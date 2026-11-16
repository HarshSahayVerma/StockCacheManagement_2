#include "Auth.h"
#include <fstream>
#include <algorithm>

const std::string AuthManager::MASTER_SECRET_KEY = "HCL@2026";

AuthManager::AuthManager(const std::string& filename) : userFile(filename) {
    loadUsers();
}

std::string AuthManager::encryptPass(const std::string& plain) {
    std::string result = plain;
    std::transform(result.begin(), result.end(), result.begin(), [](char ch) {
        return static_cast<char>(ch + 3);
    });
    return result;
}

std::string AuthManager::decryptPass(const std::string& cipher) {
    std::string result = cipher;
    std::transform(result.begin(), result.end(), result.begin(), [](char ch) {
        return static_cast<char>(ch - 3);
    });
    return result;
}

bool AuthManager::loadUsers() {
    std::ifstream inFile(userFile);
    if (!inFile.is_open()) {
        return false;
    }

    users.clear();
    UserRecord rec;
    while (inFile >> rec.username >> rec.encryptedPass >> rec.role) {
        users[rec.username] = rec;
    }
    return true;
}

bool AuthManager::saveUsers() const {
    std::ofstream outFile(userFile);
    if (!outFile.is_open()) {
        return false;
    }

    for (const auto& pair : users) {
        outFile << pair.second.username << " "
                << pair.second.encryptedPass << " "
                << pair.second.role << "\n";
    }
    return true;
}

bool AuthManager::registerUser(const std::string& uname, const std::string& pass, const std::string& role, const std::string& secretKey) {
    if (uname.empty() || pass.empty()) {
        return false;
    }
    if (role != "ADMIN" && role != "USER") {
        return false;
    }
    if (role == "ADMIN" && secretKey != MASTER_SECRET_KEY) {
        return false;
    }
    if (users.find(uname) != users.end()) {
        return false;
    }

    UserRecord rec;
    rec.username = uname;
    rec.encryptedPass = encryptPass(pass);
    rec.role = role;

    users[uname] = rec;
    return saveUsers();
}

bool AuthManager::loginUser(const std::string& uname, const std::string& pass, std::string& outRole) const {
    auto it = users.find(uname);
    if (it == users.end()) {
        return false;
    }

    std::string decrypted = decryptPass(it->second.encryptedPass);
    if (decrypted == pass) {
        outRole = it->second.role;
        return true;
    }
    return false;
}

bool AuthManager::removeUser(const std::string& targetUname, const std::string& currentAdmin) {
    if (targetUname.empty() || targetUname == currentAdmin) {
        return false;
    }

    auto it = users.find(targetUname);
    if (it == users.end()) {
        return false;
    }

    users.erase(it);
    return saveUsers();
}