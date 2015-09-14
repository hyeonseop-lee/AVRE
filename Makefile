CC=g++
CCFLAGS=-c -std=c++11 -Wall
LDFLAGS=
SRC_DIR=src
BUILD_DIR=build
TARGET=avre

SOURCES=$(wildcard $(SRC_DIR)/*.cc)
OBJECTS=$(SOURCES:$(SRC_DIR)/%.cc=$(BUILD_DIR)/%.o)

all: $(BUILD_DIR) $(BUILD_DIR)/$(TARGET)

$(BUILD_DIR):
	mkdir $(BUILD_DIR)

$(BUILD_DIR)/$(TARGET): $(OBJECTS)
	$(CC) $(LDFLAGS) -o $@ $^

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cc
	$(CC) $(CCFLAGS) -o $@ $<

clean:
	rm -rf $(BUILD_DIR)
