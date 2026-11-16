#include <gtest/gtest.h>
#include <cstdio>
#include <ctime>
#include "Auth.h"
#include "StockCache.h"

static Stock makeStock(const std::string& sym, double price, uint64_t vol, time_t ts) {
    Stock s;
    s.symbol = sym;
    s.price = price;
    s.volume = vol;
    s.timestamp = ts;
    s.accessCount = 0U;
    return s;
}

// ==================== AUTH MANAGER TESTS (1 - 12) ====================

TEST(AuthTest, 01_LoadNonExistentFile) {
    std::remove("missing_users.txt");
    AuthManager auth("missing_users.txt");
    EXPECT_FALSE(auth.loadUsers());
}

TEST(AuthTest, 02_RegisterValidUser) {
    std::remove("test_users.txt");
    AuthManager auth("test_users.txt");
    EXPECT_TRUE(auth.registerUser("investor1", "pass123", "USER"));
    std::remove("test_users.txt");
}

TEST(AuthTest, 03_RegisterDuplicateUserFails) {
    std::remove("test_users.txt");
    AuthManager auth("test_users.txt");
    EXPECT_TRUE(auth.registerUser("investor1", "pass123", "USER"));
    EXPECT_FALSE(auth.registerUser("investor1", "newpass", "USER"));
    std::remove("test_users.txt");
}

TEST(AuthTest, 04_RegisterInvalidPasswordLengthOrSpacesFails) {
    std::remove("test_users.txt");
    AuthManager auth("test_users.txt");
    EXPECT_FALSE(auth.registerUser("", "pass123", "USER"));
    EXPECT_FALSE(auth.registerUser("user1", "", "USER"));
    EXPECT_FALSE(auth.registerUser("user1", "1234", "USER"));       // < 5 chars
    EXPECT_FALSE(auth.registerUser("user1", "123456789", "USER"));  // > 8 chars
    EXPECT_FALSE(auth.registerUser("user1", "pa ss1", "USER"));     // contains space
    EXPECT_TRUE(auth.registerUser("user1", "12345", "USER"));       // exact 5 chars
    EXPECT_TRUE(auth.registerUser("user2", "12345678", "USER"));    // exact 8 chars
    std::remove("test_users.txt");
}

TEST(AuthTest, 05_RegisterInvalidRoleFails) {
    std::remove("test_users.txt");
    AuthManager auth("test_users.txt");
    EXPECT_FALSE(auth.registerUser("user1", "pass123", "GUEST"));
    EXPECT_FALSE(auth.registerUser("user1", "pass123", ""));
    std::remove("test_users.txt");
}

TEST(AuthTest, 06_RegisterAdminWithoutSecretKeyFails) {
    std::remove("test_users.txt");
    AuthManager auth("test_users.txt");
    EXPECT_FALSE(auth.registerUser("admin1", "admin123", "ADMIN"));
    EXPECT_FALSE(auth.registerUser("admin1", "admin123", "ADMIN", "WRONG_KEY"));
    std::remove("test_users.txt");
}

TEST(AuthTest, 07_RegisterAdminWithValidSecretKey) {
    std::remove("test_users.txt");
    AuthManager auth("test_users.txt");
    EXPECT_TRUE(auth.registerUser("admin1", "admin123", "ADMIN", "HCL@2026"));
    std::remove("test_users.txt");
}

TEST(AuthTest, 08_LoginSuccessAndPasswordDecryption) {
    std::remove("test_users.txt");
    AuthManager auth("test_users.txt");
    auth.registerUser("admin1", "Sec@99", "ADMIN", "HCL@2026");

    std::string role;
    EXPECT_TRUE(auth.loginUser("admin1", "Sec@99", role));
    EXPECT_EQ(role, "ADMIN");
    std::remove("test_users.txt");
}

TEST(AuthTest, 09_LoginWrongPasswordFails) {
    std::remove("test_users.txt");
    AuthManager auth("test_users.txt");
    auth.registerUser("user1", "Right1", "USER");

    std::string role;
    EXPECT_FALSE(auth.loginUser("user1", "Wrong1", role));
    std::remove("test_users.txt");
}

TEST(AuthTest, 10_LoginUnknownUserFails) {
    std::remove("test_users.txt");
    AuthManager auth("test_users.txt");
    std::string role;
    EXPECT_FALSE(auth.loginUser("ghost", "12345", role));
    std::remove("test_users.txt");
}

TEST(AuthTest, 11_PersistenceReloadFromFile) {
    std::remove("test_users.txt");
    {
        AuthManager auth1("test_users.txt");
        EXPECT_TRUE(auth1.registerUser("harsh", "hcl123", "USER"));
    }
    AuthManager auth2("test_users.txt");
    std::string role;
    EXPECT_TRUE(auth2.loginUser("harsh", "hcl123", role));
    EXPECT_EQ(role, "USER");
    std::remove("test_users.txt");
}

TEST(AuthTest, 12_RemoveUserSuccessAndGuards) {
    std::remove("test_users.txt");
    AuthManager auth("test_users.txt");
    auth.registerUser("admin1", "pass12", "ADMIN", "HCL@2026");
    auth.registerUser("user1", "pass23", "USER");

    EXPECT_FALSE(auth.removeUser("", "admin1"));
    EXPECT_FALSE(auth.removeUser("admin1", "admin1"));
    EXPECT_FALSE(auth.removeUser("unknown", "admin1"));
    EXPECT_TRUE(auth.removeUser("user1", "admin1"));

    std::string role;
    EXPECT_FALSE(auth.loginUser("user1", "pass23", role));
    std::remove("test_users.txt");
}

// ==================== STOCK CACHE TESTS (13 - 30) ====================

TEST(StockCacheTest, 13_InitialCacheIsEmpty) {
    StockCache cache;
    EXPECT_EQ(cache.getTotalRecords(), 0U);
}

TEST(StockCacheTest, 14_InsertValidStock) {
    StockCache cache;
    EXPECT_EQ(cache.insertOrUpdate(makeStock("TCS", 4100.0, 1000ULL, 1000)), 0);
    EXPECT_EQ(cache.getTotalRecords(), 1U);
}

TEST(StockCacheTest, 15_InsertEmptySymbolFails) {
    StockCache cache;
    EXPECT_EQ(cache.insertOrUpdate(makeStock("", 100.0, 10ULL, 1000)), -1);
}

TEST(StockCacheTest, 16_InsertNegativePriceFails) {
    StockCache cache;
    EXPECT_EQ(cache.insertOrUpdate(makeStock("TCS", -50.0, 10ULL, 1000)), -1);
}

TEST(StockCacheTest, 17_InsertZeroPriceAllowed) {
    StockCache cache;
    EXPECT_EQ(cache.insertOrUpdate(makeStock("FREE", 0.0, 10ULL, 1000)), 0);
}

TEST(StockCacheTest, 18_LookupExistingStockAndAccessCount) {
    StockCache cache;
    cache.insertOrUpdate(makeStock("INFY", 1800.50, 500ULL, 1000));

    Stock res;
    EXPECT_EQ(cache.lookupStock("INFY", res), 0);
    EXPECT_DOUBLE_EQ(res.price, 1800.50);
    EXPECT_EQ(res.accessCount, 1U);

    EXPECT_EQ(cache.lookupStock("INFY", res), 0);
    EXPECT_EQ(res.accessCount, 2U);
}

TEST(StockCacheTest, 19_LookupEmptyOrMissingSymbolFails) {
    StockCache cache;
    Stock res;
    EXPECT_EQ(cache.lookupStock("", res), -1);
    EXPECT_EQ(cache.lookupStock("WIPRO", res), -1);
}

TEST(StockCacheTest, 20_UpdateInPlacePreservesAccessCount) {
    StockCache cache;
    cache.insertOrUpdate(makeStock("HDFC", 1600.0, 200ULL, 1000));

    Stock res;
    cache.lookupStock("HDFC", res);
    cache.lookupStock("HDFC", res);
    EXPECT_EQ(res.accessCount, 2U);

    EXPECT_EQ(cache.insertOrUpdate(makeStock("HDFC", 1750.0, 900ULL, 2000)), 0);
    EXPECT_EQ(cache.getTotalRecords(), 1U);

    cache.lookupStock("HDFC", res);
    EXPECT_DOUBLE_EQ(res.price, 1750.0);
    EXPECT_EQ(res.volume, 900ULL);
    EXPECT_EQ(res.accessCount, 3U);
}

TEST(StockCacheTest, 21_DeleteExistingStock) {
    StockCache cache;
    cache.insertOrUpdate(makeStock("RELIANCE", 2900.0, 800ULL, 1000));
    EXPECT_EQ(cache.deleteStock("RELIANCE"), 0);
    EXPECT_EQ(cache.getTotalRecords(), 0U);
}

TEST(StockCacheTest, 22_DeleteEmptyOrMissingStockFails) {
    StockCache cache;
    EXPECT_EQ(cache.deleteStock(""), -1);
    EXPECT_EQ(cache.deleteStock("SBIN"), -1);
}

TEST(StockCacheTest, 23_MaxRecordsBoundary1000) {
    StockCache cache;
    for (size_t i = 0; i < 1000U; ++i) {
        EXPECT_EQ(cache.insertOrUpdate(makeStock("STK" + std::to_string(i), 100.0, 10ULL, 1000)), 0);
    }
    EXPECT_EQ(cache.getTotalRecords(), 1000U);
    EXPECT_EQ(cache.insertOrUpdate(makeStock("OVERFLOW", 200.0, 10ULL, 1000)), -2);
}

TEST(StockCacheTest, 24_DisplayAllWhenEmptyAndPopulated) {
    StockCache cache;
    cache.displayAllStocks();
    cache.insertOrUpdate(makeStock("TCS", 4000.0, 100ULL, 1000));
    cache.displayAllStocks();
    EXPECT_EQ(cache.getTotalRecords(), 1U);
}

TEST(StockCacheTest, 25_DisplayMostAccessedWhenNoneAndSome) {
    StockCache cache;
    cache.insertOrUpdate(makeStock("TCS", 4000.0, 100ULL, 1000));
    cache.displayMostAccessed();

    Stock res;
    cache.lookupStock("TCS", res);
    cache.displayMostAccessed();
    EXPECT_EQ(res.accessCount, 1U);
}

TEST(StockCacheTest, 26_DisplayStaleStocksWhenNoneAndSome) {
    StockCache cache;
    time_t now = std::time(nullptr);
    cache.insertOrUpdate(makeStock("FRESH", 500.0, 100ULL, now));
    cache.displayStaleStocks(60);

    cache.insertOrUpdate(makeStock("STALE", 300.0, 100ULL, now - 120));
    cache.displayStaleStocks(60);
    EXPECT_EQ(cache.getTotalRecords(), 2U);
}

TEST(StockCacheTest, 27_RemoveStaleStocksSelectiveCleanup) {
    StockCache cache;
    time_t now = std::time(nullptr);
    cache.insertOrUpdate(makeStock("FRESH1", 500.0, 100ULL, now));
    cache.insertOrUpdate(makeStock("STALE1", 300.0, 100ULL, now - 100));
    cache.insertOrUpdate(makeStock("STALE2", 200.0, 100ULL, now - 200));

    EXPECT_EQ(cache.removeStaleStocks(60), 2);
    EXPECT_EQ(cache.getTotalRecords(), 1U);
    EXPECT_EQ(cache.removeStaleStocks(60), 0);
}

TEST(StockCacheTest, 28_SaveToFileEmptyFilenameFails) {
    StockCache cache;
    EXPECT_FALSE(cache.saveToFile(""));
    EXPECT_FALSE(cache.saveToFile("nonexistent_dir/stocks.txt"));
}

TEST(StockCacheTest, 29_LoadFromFileEmptyOrMissingFails) {
    StockCache cache;
    EXPECT_FALSE(cache.loadFromFile(""));
    EXPECT_FALSE(cache.loadFromFile("missing_stocks_file.txt"));
}

TEST(StockCacheTest, 30_EndToEndSaveAndReloadCache) {
    StockCache cache1;
    cache1.insertOrUpdate(makeStock("TATAMOTORS", 950.75, 5000ULL, 1700000000));
    cache1.insertOrUpdate(makeStock("ICICIBANK", 1250.25, 3000ULL, 1700000000));

    Stock res;
    cache1.lookupStock("TATAMOTORS", res);

    EXPECT_TRUE(cache1.saveToFile("test_stocks.txt"));

    StockCache cache2;
    EXPECT_TRUE(cache2.loadFromFile("test_stocks.txt"));
    EXPECT_EQ(cache2.getTotalRecords(), 2U);

    EXPECT_EQ(cache2.lookupStock("TATAMOTORS", res), 0);
    EXPECT_DOUBLE_EQ(res.price, 950.75);
    EXPECT_EQ(res.accessCount, 2U);

    std::remove("test_stocks.txt");
}