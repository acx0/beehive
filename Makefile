# CXX = g++
CXX = clang++
CXXFLAGS += -std=c++11 -Wall -Wextra #-g #-Wconversion
CPPFLAGS += -MMD -MP -D LOGGING_ENABLED
LDFLAGS += -lserial -lboost_system -pthread

SRC = $(wildcard *.cc)
OBJ = $(SRC:%.cc=%.o)
BIN = beehive

$(BIN): $(OBJ)
	$(CXX) -o $@ $^ $(LDFLAGS)

# note: need to be part of dialout group to access /dev/ttyUSB*
# usage: make ARGS='<args>' test
# TODO: figure out how to remove LD_LIBRARY_PATH prefix
test: $(BIN)
	LD_LIBRARY_PATH=/usr/local/lib ./$(BIN) $(ARGS)

clean:
	rm -rf $(BIN) $(OBJ) *.d

.PHONY: clean test

-include $(SRC:%.cc=%.d)
