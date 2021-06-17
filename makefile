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
	install -m 644 ldmsd.timer /usr/lib/systemd/system/
	install -m 644 -D example.conf /var/lib/ldms/example.conf
ifeq (,$(wildcard /etc/ldms/ldmsd.conf))
		install -m 644 -D example.conf /etc/ldms/ldmsd.conf
endif
	gzip man/ldms.1 -c > man/ldms.1.gz
	gzip man/ldmsd.1 -c > man/ldmsd.1.gz
	gzip man/ldmsd.conf.5 -c > man/ldmsd.conf.5.gz
	install -m 644 man/ldms.1.gz /usr/share/man/man1/ldms.1.gz
	install -m 644 man/ldmsd.1.gz /usr/share/man/man1/ldmsd.1.gz
	install -m 644 man/ldmsd.conf.5.gz /usr/share/man/man5/ldmsd.conf.5.gz

uninstall:
	rm /usr/bin/$(LDMS_EXECUTABLE)
	rm /usr/bin/$(SWITCH_EXECUTABLE)
	rm /usr/lib/systemd/system/ldmsd.service
	rm /usr/lib/systemd/system/ldmsd.timer
	rm -rf /var/lib/ldms
	rm /usr/share/man/man1/ldms.1.gz
	rm /usr/share/man/man1/ldmsd.1.gz
	rm /usr/share/man/man5/ldmsd.conf.5.gz
