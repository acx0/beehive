# CXX = g++
CXX = clang++
CXXFLAGS += -std=c++11 -Wall -Wextra #-Wconversion
CPPFLAGS += -MMD -MP
LDFLAGS += -lserial -pthread

SRC = $(wildcard *.cc)
OBJ = $(SRC:%.cc=%.o)
BIN = xbee-test

$(BIN): $(OBJ)
	$(CXX) $(LDFLAGS) -o $@ $^

# note: need to be part of dialout group to access /dev/ttyUSB0
# usage: make ARGS='<args>' test
# TODO: figure out how to remove LD_LIBRARY_PATH prefix
test: $(BIN)
	LD_LIBRARY_PATH=/usr/local/lib ./$(BIN) $(ARGS)

clean:
	rm -rf $(BIN) $(OBJ) *.d

.PHONY: clean test

-include $(SRC:%.cc=%.d)
