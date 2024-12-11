CC = gcc
CFLAGS = -Wall -Wextra -Iinclude -pthread `pkg-config --cflags glib-2.0`
LIBS = `pkg-config --libs glib-2.0`  # Define the libraries to link with
TARGET = lan_speed
SRC_DIR = src
INCLUDE_DIR = include
SOURCES = $(SRC_DIR)/lan_speed.c $(SRC_DIR)/server.c $(SRC_DIR)/client.c $(SRC_DIR)/shared.c
HEADERS = $(INCLUDE_DIR)/server.h $(INCLUDE_DIR)/client.h $(INCLUDE_DIR)/shared.h

all: $(TARGET)

$(TARGET): $(SOURCES) $(HEADERS)
	$(CC) $(CFLAGS) -o $(TARGET) $(SOURCES) $(LIBS)  # Add $(LIBS) for linking

clean:
	rm -f $(TARGET)