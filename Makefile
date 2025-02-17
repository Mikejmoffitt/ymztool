APPNAME := ymztool

CP := cp
INSTALL := install
MKDIR := mkdir
RM := rm
CC := gcc
CFLAGS := -O3 -Wall -Isrc -Wno-unused-function
INSTALL_PREFIX := /usr/local/bin
ifdef SYSTEMROOT
	APPEXT := .exe
endif

SRCDIR := src

SOURCES_C := $(shell find $(SRCDIR)/ -name '*.c' -print)
SOURCES_H := $(shell find $(SRCDIR)/ -name '*.h' -print)
OBJECTS_C_DIR := cobj
OBJECTS_C := $(addprefix $(OBJECTS_C_DIR)/, $(SOURCES_C:.c=.o))

EXECNAME := $(APPNAME)$(APPEXT)

.PHONY: all clean

all: $(EXECNAME)

$(EXECNAME): $(OBJECTS_C)
	$(CC) $(CFLAGS) $(OBJECTS_C) -o $@

$(OBJECTS_C_DIR)/%.o: %.c $(SOURCES_H)
	$(MKDIR) -p $(OBJECTS_C_DIR)/$(<D)
	$(CC) -c $(CFLAGS) $< -o $@

install: $(EXECNAME)
	$(INSTALL) $< $(INSTALL_PREFIX)/

clean:
	$(RM) -rf $(OBJECTS_C_DIR)
	$(RM) -f $(EXECNAME)
