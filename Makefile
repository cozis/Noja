CXX = gcc
CC  = $(CROSS_COMPILE)$(CXX)

INCDIR  = include
CFLAGS  = -O3 -Wall -Wextra -g -I $(INCDIR)/intf -I "src/intf"
LFLAGS  = -lm

SRCDIR     = src
OBJDIR     = temp
BINDIR     = build
OUTFILE    = noja

rwildcard = $(foreach d, $(wildcard $(1:=/*)), $(call rwildcard ,$d, $2) $(filter $(subst *, %, $2), $d))

SRC          = $(call rwildcard, $(SRCDIR), *.c)
SRC_OBJECTS  = $(call rwildcard, $(SRCDIR)/objects, *.c)
SRC_COMPILER = $(call rwildcard, $(SRCDIR)/compiler, *.c)
SRC_COMMON   = $(call rwildcard, $(SRCDIR)/common, *.c)
SRC_BUILTINS = $(call rwildcard, $(SRCDIR)/builtins, *.c)
SRC_UTILS    = $(call rwildcard, $(SRCDIR)/utils, *.c)
SRC_RUNTIME  = $(call rwildcard, $(SRCDIR)/runtime, *.c)

OBJS_OBJECTS  = $(patsubst $(SRCDIR)/objects/%.c, $(OBJDIR)/objects/%.o, $(SRC_OBJECTS))
OBJS_COMPILER = $(patsubst $(SRCDIR)/compiler/%.c, $(OBJDIR)/compiler/%.o, $(SRC_COMPILER))
OBJS_COMMON   = $(patsubst $(SRCDIR)/common/%.c, $(OBJDIR)/common/%.o, $(SRC_COMMON))
OBJS_BUILTINS = $(patsubst $(SRCDIR)/builtins/%.c, $(OBJDIR)/builtins/%.o, $(SRC_BUILTINS))
OBJS_UTILS    = $(patsubst $(SRCDIR)/utils/%.c, $(OBJDIR)/utils/%.o, $(SRC_UTILS))
OBJS_RUNTIME  = $(patsubst $(SRCDIR)/runtime/%.c, $(OBJDIR)/runtime/%.o, $(SRC_UTILS))
OBJS          = $(patsubst $(SRCDIR)/%.c, $(OBJDIR)/%.o, $(SRC))


$(OBJDIR)/%.o: $(SRCDIR)/%.c
	@ echo $(shell tput setaf 3) [C] $(shell tput setaf 5) [SRC] $(shell tput setaf 7) $^
	@ mkdir -p $(@D)
	@ $(CC) $(CFLAGS) -c $^ -o $@


all: $(OBJS) build

build:
	@ mkdir -p build
	
	@ echo !==== LINKING EVERYTHING
	@ $(CC) -o $(BINDIR)/$(OUTFILE) $(OBJS) $(LFLAGS)

clean:
	@  rm -rf $(BINDIR)
	@  rm -rf $(OBJDIR)
