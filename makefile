EXECUTABLE=ldms
BUILDDIR=
OBJDIR=obj

CC=g++
SRCDIR=src
SRC:= $(shell find $(SRCDIR)/ -name '*.cpp')
OBJ=$(addprefix $(OBJDIR)/, $(subst $(SRCDIR)/, ,$(SRC:.cpp=.o)))
IDIR=include
LIBS=-I$(IDIR) -ludev

CFLAGS=-Wall -MP -MMD

$(BUILDDIR)$(EXECUTABLE): $(OBJ)
	$(CC) $(OBJ) -o $@ $(CFLAGS) $(LIBS)

-include $(OBJ:.o=.d)

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp
	mkdir -p $(dir $@)
	$(CC) -c $< -o $@ $(CFLAGS) $(LIBS)

debug: CFLAGS += -g
debug: $(BUILDDIR)$(EXECUTABLE)
