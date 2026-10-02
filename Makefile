CXX ?= g++
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra -pedantic
TARGET := network_server
SRC := src/server.cpp

.PHONY: all clean run client

all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) $(SRC) -o $(TARGET)

run: $(TARGET)
	./$(TARGET)

client:
	python3 client/client.py

clean:
	rm -f $(TARGET)
