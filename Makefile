#Compilation flags
CXXFLAGS += -std=c++2a -Wall -Wextra  -O2 -g3 $(pkg-config x11 xrandr --cflags)
LDFLAGS += $(shell pkg-config x11 xrandr --libs)

# Objects
SRC = XWindow RuntimeError XProperty Position main
OBJ = $(addsuffix .o, $(SRC))
BIN=kvmtool


all: $(BIN)

clean:
	$(RM) $(OBJ) $(LIB) $(BIN)
	$(RM) -r $(LIB_OUT)

$(BIN): $(OBJ)
	$(CXX) $(CXXFLAGS) $(OBJ) $(LDFLAGS) -o $(BIN)

install: $(BIN)
	cp -i $(BIN) /usr/bin
