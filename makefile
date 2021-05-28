LDMS_EXECUTABLE=ldmsd
SWITCH_EXECUTABLE=ldms
BUILDDIR=
OBJDIR=obj

CC=g++
SRCDIR=src
COMMON=$(SRCDIR)/config_loader.cpp $(SRCDIR)/globals.cpp $(SRCDIR)/logging.cpp
MODULES=$(SRCDIR)/modules/usb_events.cpp $(SRCDIR)/modules/lm_sensors.cpp $(SRCDIR)/modules/network.cpp
LDMS_SRC=$(SRCDIR)/ldmsd.cpp  $(COMMON) $(MODULES)
SWITCH_SRC=$(SRCDIR)/ldms.cpp $(COMMON)
IDIR=include
LIBS=-I$(IDIR) -pthread -lsensors

CFLAGS=-Wall
LDMS_FLAGS=-DLDMS_DAEMON

all: ldmsd ldms

$(LDMS_EXECUTABLE):	$(LDMS_SRC)
	$(CC) $(CFLAGS) $(LDMS_FLAGS)  $(LIBS) $(LDMS_SRC) -o $@

$(SWITCH_EXECUTABLE): $(SWITCH_SRC)
	$(CC) $(CFLAGS) $(LIBS) $(SWITCH_SRC) -o $@

debug: CFLAGS += -g
debug: all

install: $(LDMS_EXECUTABLE) $(SWITCH_EXECUTABLE)
	install $(LDMS_EXECUTABLE) /usr/bin/
	install $(SWITCH_EXECUTABLE) /usr/bin/
	install -m 644 ldmsd.service /usr/lib/systemd/system/
	install -m 644 -D example.conf /var/lib/ldms/example.conf
	install -m 644 -D example.conf /etc/ldms/ldmsd.conf

uninstall:
	rm /usr/bin/$(LDMS_EXECUTABLE)
	rm /usr/bin/$(SWITCH_EXECUTABLE)
	rm /usr/lib/systemd/system/ldmsd.service
	rm -rf /var/lib/ldms
