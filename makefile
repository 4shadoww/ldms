LDMS_EXECUTABLE=ldms
SWITCH_EXECUTABLE=switch
BUILDDIR=
OBJDIR=obj

CC=g++
SRCDIR=src
LDMS_SRC=$(SRCDIR)/ldms.cpp
SWITCH_SRC=$(SRCDIR)/switch_ldms.cpp
IDIR=include
LIBS=-I$(IDIR) -ludev

CFLAGS=-Wall

all: ldms

$(LDMS_EXECUTABLE):	$(LDMS_SRC)
	$(CC) $(CFLAGS) $(LIBS) $< -o $@

debug: CFLAGS += -g
debug: all
