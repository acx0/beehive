# CXX = g++
CXX = clang++-4.0
# TODO: -fsanitize needs to be added to compile+link commands, create separate flag to allow disabling
CXXFLAGS += -std=c++14 -Wall -Wextra -Wold-style-cast #-Weffc++ #-fsanitize=undefined #-g #-Wconversion
CPPFLAGS += -MMD -MP -D STDIO_LOGGING_ENABLED -isystem $(GTEST_DIR)/include
LDFLAGS += -lserial -lboost_system -pthread

VALGRIND_FLAGS = --leak-check=yes

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

# TODO: separate into two targets: build/run?
test: $(TEST_BIN)
	./$(TEST_BIN) $(GTEST_FLAGS) $(ARGS)

vtest: $(TEST_BIN)
	valgrind $(VALGRIND_FLAGS) ./$(TEST_BIN) $(GTEST_FLAGS) $(ARGS)

scan:
	scan-build make -j && scan-build make -j test

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
run: $(BIN)
	./$(BIN) $(ARGS)

vrun: $(BIN)
	valgrind $(VALGRIND_FLAGS) ./$(BIN) $(ARGS)

clean:
	rm -f $(BIN) $(BIN_OBJ) *.d $(TEST_BIN) $(TEST_OBJ) $(GTEST_AR) $(GTEST_OBJ)

.PHONY: clean run vrun test vtest scan

-include $(ALL_SRC:%.cc=%.d)
