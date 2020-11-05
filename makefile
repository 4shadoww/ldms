LDMS_EXECUTABLE=ldms
SWITCH_EXECUTABLE=ldmswitch
BUILDDIR=
OBJDIR=obj

CC=g++
SRCDIR=src
LDMS_SRC=$(SRCDIR)/ldms.cpp $(SRCDIR)/config_loader.cpp
SWITCH_SRC=$(SRCDIR)/ldms_switch.cpp $(SRCDIR)/config_loader.cpp
IDIR=include
LIBS=-I$(IDIR) -ludev

CFLAGS=-Wall

all: ldms ldmswitch

$(LDMS_EXECUTABLE):	$(LDMS_SRC)
	$(CC) $(CFLAGS) $(LIBS) $(LDMS_SRC) -o $@

$(SWITCH_EXECUTABLE):	$(SWITCH_SRC)
	$(CC) $(CFLAGS) $(LIBS) $(SWITCH_SRC) -o $@

debug: CFLAGS += -g
debug: all
