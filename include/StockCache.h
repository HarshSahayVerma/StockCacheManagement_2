#ifndef STOCK_CACHE_H
#define STOCK_CACHE_H

#include <string>
#include <unordered_map>
#include <cstdint>
#include <ctime>

struct Stock {
    std::string symbol;
    double price = 0.0;
    uint64_t volume = 0ULL;
    time_t timestamp = 0;
    unsigned int accessCount = 0U;
};

class StockCache {
private:
    std::unordered_map<std::string, Stock> table;
    static const size_t MAX_RECORDS = 1000U;

public:
    StockCache() = default;
    ~StockCache() = default;

    int insertOrUpdate(const Stock& stock);
    int lookupStock(const std::string& symbol, Stock& result);
    int deleteStock(const std::string& symbol);

    void displayAllStocks() const;
    void displayMostAccessed() const;
    void displayStaleStocks(int maxAgeSeconds) const;
    int removeStaleStocks(int maxAgeSeconds);

    bool saveToFile(const std::string& filename) const;
    bool loadFromFile(const std::string& filename);

    size_t getTotalRecords() const;
};

#endif