#include "StockCache.h"
#include <iostream>
#include <fstream>
#include <iomanip>

int StockCache::insertOrUpdate(const Stock& stock) {
    if (stock.symbol.empty() || stock.price < 0.0) {
        return -1;
    }

    auto it = table.find(stock.symbol);
    if (it != table.end()) {
        unsigned int prevAccess = it->second.accessCount;
        it->second = stock;
        it->second.accessCount = prevAccess;
        return 0;
    }

    if (table.size() >= MAX_RECORDS) {
        return -2;
    }

    table[stock.symbol] = stock;
    return 0;
}

int StockCache::lookupStock(const std::string& symbol, Stock& result) {
    if (symbol.empty()) {
        return -1;
    }

    auto it = table.find(symbol);
    if (it == table.end()) {
        return -1;
    }

    it->second.accessCount++;
    result = it->second;
    return 0;
}

int StockCache::deleteStock(const std::string& symbol) {
    if (symbol.empty()) {
        return -1;
    }

    auto it = table.find(symbol);
    if (it == table.end()) {
        return -1;
    }

    table.erase(it);
    return 0;
}

void StockCache::displayAllStocks() const {
    if (table.empty()) {
        std::cout << "Cache is currently empty.\n";
        return;
    }

    std::cout << "\n--- All Stocks in Cache (Total: " << getTotalRecords() << ") ---\n";
    for (const auto& pair : table) {
        const Stock& stk = pair.second;
        std::cout << stk.symbol << " | Price: " << std::fixed << std::setprecision(2) << stk.price
                  << " | Vol: " << stk.volume
                  << " | Time: " << stk.timestamp
                  << " | Accesses: " << stk.accessCount << "\n";
    }
}

void StockCache::displayMostAccessed() const {
    std::cout << "\n--- Most Accessed Stocks ---\n";
    bool found = false;
    for (const auto& pair : table) {
        if (pair.second.accessCount > 0U) {
            std::cout << pair.second.symbol << " | Access Count = " << pair.second.accessCount << "\n";
            found = true;
        }
    }
    if (!found) {
        std::cout << "No accessed stocks yet.\n";
    }
}

void StockCache::displayStaleStocks(int maxAgeSeconds) const {
    time_t now = std::time(nullptr);
    std::cout << "\n--- Stale Stocks (Older than " << maxAgeSeconds << "s) ---\n";
    bool found = false;

    for (const auto& pair : table) {
        if ((now - pair.second.timestamp) > maxAgeSeconds) {
            std::cout << pair.second.symbol << " | Price: " << std::fixed << std::setprecision(2)
                      << pair.second.price << "\n";
            found = true;
        }
    }
    if (!found) {
        std::cout << "No stale stocks found.\n";
    }
}

int StockCache::removeStaleStocks(int maxAgeSeconds) {
    time_t now = std::time(nullptr);
    int removedCount = 0;

    for (auto it = table.begin(); it != table.end(); ) {
        if ((now - it->second.timestamp) > maxAgeSeconds) {
            it = table.erase(it);
            removedCount++;
        } else {
            ++it;
        }
    }
    return removedCount;
}

bool StockCache::saveToFile(const std::string& filename) const {
    if (filename.empty()) {
        return false;
    }

    std::ofstream outFile(filename);
    if (!outFile.is_open()) {
        return false;
    }

    for (const auto& pair : table) {
        const Stock& stk = pair.second;
        outFile << stk.symbol << " "
                << std::fixed << std::setprecision(2) << stk.price << " "
                << stk.volume << " "
                << stk.timestamp << " "
                << stk.accessCount << "\n";
    }
    return true;
}

bool StockCache::loadFromFile(const std::string& filename) {
    if (filename.empty()) {
        return false;
    }

    std::ifstream inFile(filename);
    if (!inFile.is_open()) {
        return false;
    }

    Stock stk;
    while (inFile >> stk.symbol >> stk.price >> stk.volume >> stk.timestamp >> stk.accessCount) {
        insertOrUpdate(stk);
    }
    return true;
}

size_t StockCache::getTotalRecords() const {
    return table.size();
}