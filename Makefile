CC ?= gcc
CFLAGS ?= -O2 -Wall -Wextra -pedantic
LDFLAGS ?=
LDLIBS ?= -lcurl
PREFIX ?= /usr/local
BINDIR ?= $(PREFIX)/bin

TARGET = dxlapi
SRC = main.c batch.c config.c http_client.c json_util.c log.c position.c rate_limit.c telemetry.c udp.c
OBJ = $(SRC:.c=.o)
DEP = $(SRC:.c=.d)

.PHONY: all clean install uninstall

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(OBJ) $(LDLIBS)

%.o: %.c
	$(CC) $(CFLAGS) -MMD -MP -c -o $@ $<

install: $(TARGET)
	install -d $(DESTDIR)$(BINDIR)
	install -m 0755 $(TARGET) $(DESTDIR)$(BINDIR)/$(TARGET)

uninstall:
	rm -f $(DESTDIR)$(BINDIR)/$(TARGET)

clean:
	rm -f $(TARGET) $(OBJ) $(DEP)

-include $(DEP)
