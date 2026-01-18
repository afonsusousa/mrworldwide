CC ?= gcc
CFLAGS = -std=c23 -Wall -Wextra -D_DEFAULT_SOURCE -g

TARGET = mrworldwide
SRCS = watcher.c

ifeq ($(OS),Windows_NT)
	# Windows
	CFLAGS += -Iwindows/hidapi
	LIBS = windows/hidapi/hidapi.lib -luser32
	TARGET_BIN = $(TARGET).exe
	POST_BUILD = cp windows/hidapi/hidapi.dll . 2>/dev/null || copy windows\hidapi\hidapi.dll . 2>/dev/null || true
else
	# Linux
	UNAME_S := $(shell uname -s)
	ifeq ($(UNAME_S),Linux)
		CFLAGS += $(shell pkg-config --cflags hidapi-hidraw)
		LIBS = $(shell pkg-config --libs hidapi-hidraw)
		TARGET_BIN = $(TARGET)
		POST_BUILD = @echo "Build complete."
	endif
endif

all: $(TARGET_BIN)

$(TARGET_BIN): $(SRCS)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)
	$(POST_BUILD)

clean:
	rm -f $(TARGET) $(TARGET).exe

re: clean all