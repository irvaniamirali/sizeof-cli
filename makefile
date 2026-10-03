CC = cc
CFLAGS = -O3 -Wall -Wextra -Wpedantic
CPPFLAGS = -DDEFAULT_HEADERS_FILE=\"/etc/sizeof/headers.csrc\"

TARGET = sizeof
SOURCE = main.c

PREFIX = /usr
BINDIR = $(PREFIX)/bin
SYSCONFDIR = /etc/sizeof
HEADERS_FILE = $(SYSCONFDIR)/headers.csrc

.PHONY: all install uninstall clean

all: $(TARGET)

$(TARGET): $(SOURCE)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(SOURCE) -o $(TARGET)

install: $(TARGET)
	install -d $(DESTDIR)$(BINDIR)
	install -d $(DESTDIR)$(SYSCONFDIR)
	install -m 755 $(TARGET) $(DESTDIR)$(BINDIR)/$(TARGET)
	install -m 644 headers.csrc $(DESTDIR)$(HEADERS_FILE)

uninstall:
	rm -f $(DESTDIR)$(BINDIR)/$(TARGET)
	rm -f $(DESTDIR)$(HEADERS_FILE)
	rmdir $(DESTDIR)$(SYSCONFDIR) 2>/dev/null || true

clean:
	rm -f $(TARGET)