# In order to execute this "Makefile" just type "make" or "make all"
.PHONY: all clean

CPP		= g++
CFLAGS	= -g -Wall -c
LFLAGS	= -fsanitize=address -g3

SRC_DIR	= src
BUILD_DIR = build/release
DIRS	= named_pipes/* out/*

OBJS	= $(BUILD_DIR)/sniffer.o $(BUILD_DIR)/workers.o
EXECUTABLES	= $(BUILD_DIR)/sniffer $(BUILD_DIR)/workers

all: dir $(OBJS) $(EXECUTABLES)

dir:
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/sniffer.o:
	$(CPP) $(CFLAGS) $(SRC_DIR)/sniffer.cpp -o $@

$(BUILD_DIR)/workers.o:
	$(CPP) $(CFLAGS) $(SRC_DIR)/workers.cpp -o $@

$(BUILD_DIR)/sniffer:
	$(CPP) $(LFLAGS) $(BUILD_DIR)/sniffer.o -o $@

$(BUILD_DIR)/workers:
	$(CPP) $(LFLAGS) $(BUILD_DIR)/workers.o -o $@

# Clean things
clean:
	rm -r -f $(BUILD_DIR)/* $(DIRS)
