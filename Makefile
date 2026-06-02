CC = gcc
CFLAGS = -Wall -O3

# Use pkg-config to scan the FFmpeg library paths on your system.
# In Fedora, pkg-config can sometimes omit the path -I /usr/include/ffmpeg.
# Therefore, we are also adding Fedora's custom fallback path to CFLAGS.
PKG_CFLAGS = $(shell pkg-config --cflags libavformat libavcodec libswscale libavutil 2>/dev/null)
CFLAGS += $(PKG_CFLAGS) 
CFLAGS += -I/usr/include -I/usr/include/ffmpeg

# Define the dynamic libraries (Shared Objects) to connect to.
LIBS = $(shell pkg-config --libs libavformat libavcodec libswscale libavutil 2>/dev/null || echo "-lavformat -lavcodec -lswscale -lavutil")

TARGET = pixelator
SRC = main.c

all: $(TARGET)

# Compilation and Dependency Linking Phase
$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC) $(LIBS)

# Cleaning Phase
clean:
	rm -f $(TARGET)