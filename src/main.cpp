#include <iostream>
#include <limits>
#include <string>
#include "Auth.h"
#include "StockCache.h"

#ifdef _WIN32
#include <conio.h>
#else
#include <termios.h>
#include <unistd.h>
#endif

static void clearInput() {
    std::cin.clear();
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}

static bool readNoSpaceInput(const std::string& prompt, std::string& outStr) {
    std::cout << prompt;
    std::getline(std::cin >> std::ws, outStr);
    if (outStr.empty() || outStr.find(' ') != std::string::npos) {
        return false;
    }
    return true;
}

// Cross-platform masked password input showing '*' for each typed character
static std::string readMaskedPassword(const std::string& prompt) {
    std::cout << prompt;
    std::cout.flush();
    std::string pass;

#ifdef _WIN32
    while (true) {
        int ch = _getch();
        if (ch == '\r' || ch == '\n') {
            std::cout << '\n';
            break;
        }
        if (ch == '\b' || ch == 127) {
            if (!pass.empty()) {
                pass.pop_back();
                std::cout << "\b \b";
            }
        } else if (ch >= 32 && ch <= 126) {
            pass.push_back(static_cast<char>(ch));
            std::cout << '*';
        }
    }
#else
    termios oldt{};
    tcgetattr(STDIN_FILENO, &oldt);
    termios newt = oldt;
    newt.c_lflag &= ~static_cast<tcflag_t>(ECHO | ICANON);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);

    char ch = '\0';
    while (read(STDIN_FILENO, &ch, 1) == 1) {
        if (ch == '\n' || ch == '\r') {
            std::cout << '\n';
            break;
        }
        if (ch == 127 || ch == '\b') {
            if (!pass.empty()) {
                pass.pop_back();
                std::cout << "\b \b";
                std::cout.flush();
            }
        } else if (ch >= 32 && ch <= 126) {
            pass.push_back(ch);
            std::cout << '*';
            std::cout.flush();
        }
    }
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
#endif
    return pass;
}

static void runAdminMenu(StockCache& cache, AuthManager& auth, const std::string& currentAdmin) {
    int choice = -1;
    while (true) {
        std::cout << "\n=== ADMIN (EDITOR) MENU ===\n"
                  << "1. Add / Update Stock\n"
                  << "2. Search Stock\n"
                  << "3. Delete Stock\n"
                  << "4. Display All Stocks\n"
                  << "5. Display Most Accessed Stocks\n"
                  << "6. Save Cache (stocks.txt)\n"
                  << "7. Load Cache (stocks.txt)\n"
                  << "8. Display Stale Stocks (>60s)\n"
                  << "9. Remove Stale Stocks (>60s)\n"
                  << "10. Remove User\n"
                  << "0. Logout\n"
                  << "Enter Choice: ";

        if (!(std::cin >> choice)) {
            clearInput();
            std::cout << "Invalid input.\n";
            continue;
        }
        clearInput();

        if (choice == 0) {
            std::cout << "Logging out...\n";
            break;
        }

        switch (choice) {
            case 1: {
                Stock stk;
                if (!readNoSpaceInput("Enter Symbol: ", stk.symbol)) {
                    std::cout << "Invalid Symbol (no spaces allowed).\n";
                    break;
                }
                std::cout << "Enter Price: ";
                if (!(std::cin >> stk.price) || stk.price < 0.0) {
                    clearInput();
                    std::cout << "Invalid price.\n";
                    break;
                }
                std::cout << "Enter Volume: ";
                if (!(std::cin >> stk.volume)) {
                    clearInput();
                    std::cout << "Invalid volume.\n";
                    break;
                }
                clearInput();
                stk.timestamp = std::time(nullptr);
                stk.accessCount = 0U;

                int status = cache.insertOrUpdate(stk);
                if (status == 0) {
                    std::cout << "Stock Added/Updated Successfully.\n";
                } else if (status == -2) {
                    std::cout << "Maximum Record Limit Reached.\n";
                } else {
                    std::cout << "Operation Failed.\n";
                }
                break;
            }
            case 2: {
                std::string sym;
                Stock result;
                if (!readNoSpaceInput("Enter Symbol: ", sym)) {
                    std::cout << "Invalid Symbol.\n";
                    break;
                }
                if (cache.lookupStock(sym, result) == 0) {
                    std::cout << "\nSymbol       : " << result.symbol
                              << "\nPrice        : " << result.price
                              << "\nVolume       : " << result.volume
                              << "\nTimestamp    : " << result.timestamp
                              << "\nAccess Count : " << result.accessCount << "\n";
                } else {
                    std::cout << "Stock Not Found.\n";
                }
                break;
            }
            case 3: {
                std::string sym;
                if (!readNoSpaceInput("Enter Symbol: ", sym)) {
                    std::cout << "Invalid Symbol.\n";
                    break;
                }
                if (cache.deleteStock(sym) == 0) {
                    std::cout << "Stock Deleted Successfully.\n";
                } else {
                    std::cout << "Stock Not Found.\n";
                }
                break;
            }
            case 4:
                cache.displayAllStocks();
                break;
            case 5:
                cache.displayMostAccessed();
                break;
            case 6:
                if (cache.saveToFile("data/stocks.txt")) {
                    std::cout << "Cache Saved Successfully.\n";
                } else {
                    std::cout << "Save Failed.\n";
                }
                break;
            case 7:
                if (cache.loadFromFile("data/stocks.txt")) {
                    std::cout << "Cache Loaded Successfully.\n";
                } else {
                    std::cout << "Load Failed.\n";
                }
                break;
            case 8:
                cache.displayStaleStocks(60);
                break;
            case 9: {
                int removed = cache.removeStaleStocks(60);
                std::cout << "Stale Stocks Removed (" << removed << " records).\n";
                break;
            }
            case 10: {
                std::string targetUser;
                if (!readNoSpaceInput("Enter Username to Remove: ", targetUser)) {
                    std::cout << "Invalid Username.\n";
                    break;
                }
                if (auth.removeUser(targetUser, currentAdmin)) {
                    std::cout << "User '" << targetUser << "' Removed Successfully.\n";
                } else {
                    std::cout << "Removal Failed (User not found or cannot delete logged-in admin).\n";
                }
                break;
            }
            default:
                std::cout << "Invalid Choice.\n";
                break;
        }
    }
}

static void runUserMenu(StockCache& cache) {
    int choice = -1;
    while (true) {
        std::cout << "\n=== USER (INVESTOR - READ ONLY) MENU ===\n"
                  << "1. Search Stock\n"
                  << "2. Display All Stocks\n"
                  << "3. Display Most Accessed Stocks\n"
                  << "4. Display Stale Stocks (>60s)\n"
                  << "0. Logout\n"
                  << "Enter Choice: ";

        if (!(std::cin >> choice)) {
            clearInput();
            std::cout << "Invalid input.\n";
            continue;
        }
        clearInput();

        if (choice == 0) {
            std::cout << "Logging out...\n";
            break;
        }

        switch (choice) {
            case 1: {
                std::string sym;
                Stock result;
                if (!readNoSpaceInput("Enter Symbol: ", sym)) {
                    std::cout << "Invalid Symbol.\n";
                    break;
                }
                if (cache.lookupStock(sym, result) == 0) {
                    std::cout << "\nSymbol       : " << result.symbol
                              << "\nPrice        : " << result.price
                              << "\nVolume       : " << result.volume
                              << "\nTimestamp    : " << result.timestamp
                              << "\nAccess Count : " << result.accessCount << "\n";
                } else {
                    std::cout << "Stock Not Found.\n";
                }
                break;
            }
            case 2:
                cache.displayAllStocks();
                break;
            case 3:
                cache.displayMostAccessed();
                break;
            case 4:
                cache.displayStaleStocks(60);
                break;
            default:
                std::cout << "Invalid Choice.\n";
                break;
        }
    }
}

int main() {
    AuthManager auth("data/users.txt");
    StockCache cache;
    cache.loadFromFile("data/stocks.txt");

    int authChoice = -1;
    while (true) {
        std::cout << "\n==============================\n"
                  << "  STOCK CACHE SYSTEM (C++)\n"
                  << "==============================\n"
                  << "1. Login\n"
                  << "2. Register as User (Investor)\n"
                  << "3. Register as Editor (Admin)\n"
                  << "0. Exit\n"
                  << "Enter Choice: ";

        if (!(std::cin >> authChoice)) {
            clearInput();
            std::cout << "Invalid input.\n";
            continue;
        }
        clearInput();

        if (authChoice == 0) {
            cache.saveToFile("data/stocks.txt");
            std::cout << "Exiting Program...\n";
            break;
        }

        if (authChoice == 1) {
            std::string uname, role;
            if (!readNoSpaceInput("Username: ", uname)) {
                std::cout << "Invalid input (spaces not allowed).\n";
                continue;
            }
            std::string pass = readMaskedPassword("Password: ");

            if (auth.loginUser(uname, pass, role)) {
                std::cout << "\nLogin Successful! Role: " << role << "\n";
                if (role == "ADMIN") {
                    runAdminMenu(cache, auth, uname);
                } else {
                    runUserMenu(cache);
                }
            } else {
                std::cout << "Invalid Username or Password.\n";
            }
        } else if (authChoice == 2) {
            std::string uname;
            if (!readNoSpaceInput("Enter New Username (no spaces): ", uname)) {
                std::cout << "Registration Failed (Username cannot contain spaces).\n";
                continue;
            }
            std::string pass = readMaskedPassword("Enter New Password (5-8 chars, no spaces): ");
            if (pass.find(' ') != std::string::npos) {
                std::cout << "Registration Failed (Password cannot contain spaces).\n";
                continue;
            }
            if (pass.length() < 5 || pass.length() > 8) {
                std::cout << "Registration Failed (Password must be between 5 and 8 characters).\n";
                continue;
            }

            if (auth.registerUser(uname, pass, "USER")) {
                std::cout << "User Registration Successful! You can now log in.\n";
            } else {
                std::cout << "Registration Failed (Username already exists).\n";
            }
        } else if (authChoice == 3) {
            std::string uname, secretKey;
            if (!readNoSpaceInput("Enter New Editor Username (no spaces): ", uname)) {
                std::cout << "Registration Failed (Username cannot contain spaces).\n";
                continue;
            }
            std::string pass = readMaskedPassword("Enter New Password (5-8 chars, no spaces): ");
            if (pass.find(' ') != std::string::npos) {
                std::cout << "Registration Failed (Password cannot contain spaces).\n";
                continue;
            }
            if (pass.length() < 5 || pass.length() > 8) {
                std::cout << "Registration Failed (Password must be between 5 and 8 characters).\n";
                continue;
            }
            if (!readNoSpaceInput("Enter Master Admin Secret Key: ", secretKey)) {
                std::cout << "Registration Failed (Invalid Secret Key).\n";
                continue;
            }

            if (auth.registerUser(uname, pass, "ADMIN", secretKey)) {
                std::cout << "Editor (ADMIN) Registration Successful! You can now log in.\n";
            } else {
                std::cout << "Registration Failed (Invalid Secret Key or Username exists).\n";
            }
        } else {
            std::cout << "Invalid Choice.\n";
        }
    }
    return 0;
}