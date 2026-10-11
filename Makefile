CC ?= gcc
QUICKJS_DIR ?= $(HOME)/git/quickjs

#CFLAGS ?= -O2 -Wall -Wextra
CFLAGS += -fPIC -I$(QUICKJS_DIR)
LDFLAGS = -shared
LDLIBS = -lcurl

TARGET_DIR = libcurl-wrapper
TARGET = $(TARGET_DIR)/curl_sync.so
SRCS = $(TARGET_DIR)/curl_sync.c

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(SRCS)
	@mkdir -p $(TARGET_DIR)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LDLIBS)

clean:
	rm -f $(TARGET)
