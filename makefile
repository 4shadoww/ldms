LDMS_EXECUTABLE=ldms
SWITCH_EXECUTABLE=ldmswitch
BUILDDIR=
OBJDIR=obj

CC=g++
SRCDIR=src
MODULES=$(SRCDIR)/modules/usb_events.cpp
LDMS_SRC=$(SRCDIR)/ldms.cpp $(SRCDIR)/config_loader.cpp $(MODULES)
SWITCH_SRC=$(SRCDIR)/ldms_switch.cpp $(SRCDIR)/config_loader.cpp $(MODULES)
IDIR=include
LIBS=-I$(IDIR) -pthread

CFLAGS=-Wall

all: ldms ldmswitch

$(LDMS_EXECUTABLE):	$(LDMS_SRC)
	$(CC) $(CFLAGS) $(LIBS) $(LDMS_SRC) -o $@

$(SWITCH_EXECUTABLE):	$(SWITCH_SRC)
	$(CC) $(CFLAGS) $(LIBS) $(SWITCH_SRC) -o $@

debug: CFLAGS += -g
debug: all
