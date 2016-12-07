# CXX = g++
CXX = clang++
CXXFLAGS += -std=c++11 -Wall -Wextra #-fsanitize=undefined #-g #-Wconversion
CPPFLAGS += -MMD -MP -D STDIO_LOGGING_ENABLED -isystem $(GTEST_DIR)/include
LDFLAGS += -lserial -lboost_system -pthread

GTEST_FLAGS = --gtest_color=yes
GTEST_DIR = /usr/src/googletest/googletest
GTEST_AR = gtest.a gtest_main.a
GTEST_OBJ = gtest-all.o gtest_main.o

ALL_SRC = $(wildcard *.cc)
TEST_SRC = $(wildcard *_test.cc)
BIN_SRC = $(filter-out $(TEST_SRC), $(ALL_SRC))

BIN_OBJ = $(BIN_SRC:%.cc=%.o)
TEST_OBJ = $(TEST_SRC:%.cc=%.o)
TEST_DEP_OBJ = $(filter-out main.o, $(BIN_OBJ))

BIN = beehive
TEST_BIN = beehive-tests

$(BIN): $(BIN_OBJ)
	$(CXX) -o $@ $^ $(LDFLAGS)

test: $(TEST_BIN)
	./$(TEST_BIN) $(GTEST_FLAGS) $(ARGS)

$(TEST_BIN): $(TEST_DEP_OBJ) $(TEST_OBJ) $(GTEST_AR)
	$(CXX) -o $@ $(TEST_DEP_OBJ) $(TEST_OBJ) gtest_main.a $(LDFLAGS)

gtest.a: gtest-all.o
	$(AR) $(ARFLAGS) $@ $^

gtest_main.a: $(GTEST_OBJ)
	$(AR) $(ARFLAGS) $@ $^

gtest-all.o: $(GTEST_DIR)
	$(CXX) -I$(GTEST_DIR) -c $(GTEST_DIR)/src/gtest-all.cc

gtest_main.o: $(GTEST_DIR)
	$(CXX) -I$(GTEST_DIR) -c $(GTEST_DIR)/src/gtest_main.cc

# note: need to be part of dialout group to access /dev/ttyUSB*
# usage: make ARGS='<args>' test
# TODO: figure out how to remove LD_LIBRARY_PATH prefix
run: $(BIN)
	LD_LIBRARY_PATH=/usr/local/lib ./$(BIN) $(ARGS)

clean:
	rm -f $(BIN) $(BIN_OBJ) *.d $(TEST_BIN) $(TEST_OBJ) $(GTEST_AR) $(GTEST_OBJ)

.PHONY: clean run test

-include $(ALL_SRC:%.cc=%.d)
