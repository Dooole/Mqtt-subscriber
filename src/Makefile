INSTALL_DIR ?= /usr/sbin/

CC ?= gcc
INSTALL_BIN ?= install -m 0755
RM ?= rm -f

CFLAGS += -std=c99 -Wall -D_DEFAULT_SOURCE -D_GNU_SOURCE -I/usr/local/include
LDFLAGS += -L/usr/local/lib

LDFLAGS +=-largp \
-lmosquitto \
-luci \
-lcurl \
-ljson-c \
-lsqlite3

SRC := $(wildcard *.c)
OBJ := $(SRC:.c=.o)

TARGET := mqttsub

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $^ $(LDFLAGS) -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	$(RM) $(TARGET) $(OBJ)

install:
	$(INSTALL_BIN) $(TARGET) $(INSTALL_DIR)
