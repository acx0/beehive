# CXX = g++
CXX = clang++
CXX_STD = -std=c++14
CXXFLAGS += $(CXX_STD) -Wall -Wextra -Wold-style-cast $(SANITIZE_FLAGS) -O1 -g -fno-omit-frame-pointer

# set gtest include directories as system directories to prevent warnings in gtest headers
CPPFLAGS += -MMD -MP -D STDIO_LOGGING_ENABLED -isystem $(GTEST_DIR)/include -isystem $(GMOCK_DIR)/include
LDFLAGS += -lserial -lboost_system -pthread $(SANITIZE_FLAGS)

# available sanitizers: address,leak,memory,thread,undefined
# note: disable when using valgrind, certain sanitizers conflict
SANITIZE_FLAGS = -fsanitize=address,leak,undefined

VALGRIND_FLAGS = --leak-check=yes

GTEST_FLAGS = --gtest_color=yes
GTEST_DIR = /usr/src/googletest/googletest
GMOCK_DIR = /usr/src/googletest/googlemock
GTEST_HEADERS = $(GTEST_DIR)/include/gtest/*.h $(GTEST_DIR)/include/gtest/internal/*.h
GMOCK_HEADERS = $(GMOCK_DIR)/include/gmock/*.h $(GMOCK_DIR)/include/gmock/internal/*.h $(GTEST_HEADERS)
GTEST_SRC = $(GTEST_DIR)/src/*.cc $(GTEST_DIR)/src/*.h $(GTEST_HEADERS)
GMOCK_SRC = $(GMOCK_DIR)/src/*.cc $(GMOCK_HEADERS)
GTEST_OBJ = gtest-all.o gmock-all.o gmock_main.o

ALL_SRC = $(wildcard *.cc)
TEST_SRC = $(wildcard *_test.cc)
BIN_SRC = $(filter-out $(TEST_SRC), $(ALL_SRC))

BIN_OBJ = $(BIN_SRC:%.cc=%.o)
TEST_OBJ = $(TEST_SRC:%.cc=%.o)
TEST_DEP_OBJ = $(filter-out main.o, $(BIN_OBJ))

BIN = beehive
TEST_BIN = beehive-tests

$(BIN): $(BIN_OBJ)
	$(CXX) $^ $(LDFLAGS) -o $@

.PHONY: test
test: $(TEST_BIN)
	./$(TEST_BIN) $(GTEST_FLAGS) $(ARGS)

.PHONY: vtest
vtest: $(TEST_BIN)
	valgrind $(VALGRIND_FLAGS) ./$(TEST_BIN) $(GTEST_FLAGS) $(ARGS)

$(TEST_BIN): $(TEST_DEP_OBJ) $(TEST_OBJ) gmock_main.a
	$(CXX) $^ $(LDFLAGS) -o $@

# for using custom main function to run tests
# gmock.a: gmock-all.o gtest-all.o
# 	$(AR) $(ARFLAGS) $@ $^

# uses gtest provided main function to run tests
gmock_main.a: $(GTEST_OBJ)
	$(AR) $(ARFLAGS) $@ $^

gtest-all.o: $(GTEST_SRC)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -I$(GTEST_DIR) -c $(GTEST_DIR)/src/gtest-all.cc

gmock-all.o: $(GMOCK_SRC)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -I$(GTEST_DIR) -I$(GMOCK_DIR) -c $(GMOCK_DIR)/src/gmock-all.cc

gmock_main.o: $(GMOCK_SRC)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -I$(GTEST_DIR) -I$(GMOCK_DIR) -c $(GMOCK_DIR)/src/gmock_main.cc

# generate tag file of source files as well as any referenced system headers
tags: $(BIN_SRC)
	$(CXX) $(CXX_STD) -M $^ \
	    | tr '\\ ' '\n' \
	    | sed -e '/^$$/d' -e '/\.o:[ \t]*$$/d' \
	    | ctags -L - --c++-kinds=+px --fields=+aiS --extra=+q

# note: need to be part of dialout group to access /dev/ttyUSB*
# usage: make ARGS='<args>' run
.PHONY: run
run: $(BIN)
	./$(BIN) $(ARGS)

.PHONY: vrun
vrun: $(BIN)
	valgrind $(VALGRIND_FLAGS) ./$(BIN) $(ARGS)

.PHONY: clean
clean:
	rm -f $(BIN) $(BIN_OBJ) *.d $(TEST_BIN) $(TEST_OBJ) $(GTEST_OBJ) gmock_main.a *.plist compile_commands.json tags

-include $(ALL_SRC:%.cc=%.d)
