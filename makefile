NAME = WLAN
DESCRIPTION = "WLAN Configuration Utility"
ARCHIVE = YES
COMPRESSED = YES
COMPRESSED_MODE = zx0

GIT_TAG = $(shell git describe --tags --abbrev=0 2>/dev/null || echo "v0.0.0")
GIT_SHA = $(shell git rev-parse --short HEAD 2>/dev/null || echo "unknown")

CFLAGS = -Wall -Wextra -Oz -DGIT_TAG=\"$(GIT_TAG)\" -DGIT_SHA=\"$(GIT_SHA)\"

transfer:
	tilp -n bin/$(NAME).8xp

cat: clean all transfer

include $(shell cedev-config --makefile)
