CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -g -Iinclude
COVFLAGS = --coverage

SRCS = src/Auth.cpp src/StockCache.cpp src/main.cpp
LIB_SRCS = src/Auth.cpp src/StockCache.cpp
TARGET = stock_app
TEST_TARGET = run_tests

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SRCS)

test: $(LIB_SRCS) tests/test_cache.cpp
	rm -f *.gcno *.gcda *.gcov
	$(CXX) $(CXXFLAGS) $(COVFLAGS) -o $(TEST_TARGET) $(LIB_SRCS) tests/test_cache.cpp -lgtest -lgtest_main -pthread
	./$(TEST_TARGET)

coverage: test
	gcov -r $(TEST_TARGET)-Auth $(TEST_TARGET)-StockCache

cppcheck:
	cppcheck --enable=all --inconclusive --check-level=exhaustive --std=c++17 -Iinclude --suppress=missingIncludeSystem src/

valgrind: $(TARGET)
	valgrind --leak-check=full --show-leak-kinds=all ./$(TARGET)

clean:
	rm -f $(TARGET) $(TARGET).exe $(TEST_TARGET) $(TEST_TARGET).exe *.o *.gcno *.gcda *.gcov test_users.txt test_stocks.txt