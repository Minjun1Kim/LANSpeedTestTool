CC = gcc
CFLAGS = -Wall -Wextra -Iinclude -pthread
TARGET = lan_speed
SRC_DIR = src
INCLUDE_DIR = include
SOURCES = $(SRC_DIR)/lan_speed.c $(SRC_DIR)/server.c $(SRC_DIR)/client.c
HEADERS = $(INCLUDE_DIR)/server.h $(INCLUDE_DIR)/client.h $(INCLUDE_DIR)/shared.h

all: $(TARGET)

$(TARGET): $(SOURCES) $(HEADERS)
	$(CC) $(CFLAGS) -o $(TARGET) $(SOURCES)

clean:
	rm -f $(TARGET)