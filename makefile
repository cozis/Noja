
# ===================== #
# === Configuration === #
# ===================== #

# Destination folder of the final artifacts 
# (it must exist beforehand)
OUTDIR = .

# Base folder of the source tree
SRCDIR = src

# Temporary directory where all intermediary 
# artifacts will be stored (if it doesn't exist, 
# it will be created). It's important that this 
# folder isn't used for something other than 
# holding object files since it will be deleted 
# when doing `make clean`.
OBJDIR = cache

# Temporary directory where execution coverage
# reports are stored. If this directory doesn't
# exist, it will be created. This folder is
# deleted by `make clean`.
REPORTDIR = report

# Source directory for each program to be build
# They are relative to the SRCDIR folder.
LIB_SUBDIR = lib
CLI_SUBDIR = cli
TST_SUBDIR = test

# Resulting file names
LIB_FNAME = libnoja.a
CLI_FNAME = noja
TST_FNAME = test

# Where the build programs will be moved when
# installation occurres.
CLI_INSTALLDIR = /bin
LIB_INSTALLDIR = /usr/local

# Default programs
CC = gcc
AR = ar

# Program flags
CFLAGS = -Wall -Wextra
LFLAGS = -lm

# Build the library with valgrind support.
# Can be one of: YES, NO
USING_VALGRIND = NO

# May be one of: COVERAGE, RELEASE, DEBUG
BUILD_MODE = RELEASE

# Changing the BUILD_MODE will add one of the
# following set of flags to the CFLAGS and 
# LFLAGS. If the BUILD_MODE is COVERAGE, then 
# the flags are also determined based on the CC.
CFLAGS_DEBUG = -DDEBUG -g
LFLAGS_DEBUG =

CFLAGS_RELEASE = -DNDEBUG -O3
LFLAGS_RELEASE =

CFLAGS_COVERAGE_GCC = -fprofile-arcs -ftest-coverage
LFLAGS_COVERAGE_GCC = -lgcov
CFLAGS_COVERAGE_CLANG = -fprofile-instr-generate -fcoverage-mapping
LFLAGS_COVERAGE_CLANG =
# NOTE: To support BUILD_MODE=COVERAGE for a new compiler,
#       just define CFLAGS_COVERAGE_xxx and LFLAGS_COVERAGE_xxx,
#       where xxx is the uppercase version of the compiler
#       name (the value of CC).

# ===================== #
# === Variables ======= #
# ===================== #

# Auxiliary function
rwildcard = $(foreach d, $(wildcard $(1:=/*)), $(call rwildcard ,$d, $2) $(filter $(subst *, %, $2), $d))
uppercase = $(shell echo '$1' | tr '[:lower:]' '[:upper:]')

# Add the flags specific to the build mode.
ifeq ($(BUILD_MODE),COVERAGE)
	UPPERCASE_CC = $(call uppercase,$(CC))
	CFLAGS += ${CFLAGS_COVERAGE_$(UPPERCASE_CC)}
	LFLAGS += ${LFLAGS_COVERAGE_$(UPPERCASE_CC)}
else
	CFLAGS += ${CFLAGS_$(BUILD_MODE)}
	LFLAGS += ${LFLAGS_$(BUILD_MODE)}
endif

# Add the valgrind flag if requested.
ifeq ($(USING_VALGRIND),YES)
	CFLAGS += -DUSING_VALGRIND=1
endif

# Absolute path of each program's source tree
LIB_SRCDIR = $(SRCDIR)/$(LIB_SUBDIR)
CLI_SRCDIR = $(SRCDIR)/$(CLI_SUBDIR)
TST_SRCDIR = $(SRCDIR)/$(TST_SUBDIR)

# Each program that is being build uses as object
# cache a subfolder of OBJDIR called like the it's
# source tree base folder in SRCDIR.
LIB_OBJDIR = $(OBJDIR)/$(LIB_SUBDIR)
CLI_OBJDIR = $(OBJDIR)/$(CLI_SUBDIR)
TST_OBJDIR = $(OBJDIR)/$(TST_SUBDIR)

LIB_CFILES = $(call rwildcard, $(LIB_SRCDIR), *.c)
LIB_HFILES = $(call rwildcard, $(LIB_SRCDIR), *.h)
LIB_OFILES = $(patsubst $(LIB_SRCDIR)/%.c, $(LIB_OBJDIR)/%.o, $(LIB_CFILES))

CLI_CFILES = $(call rwildcard, $(CLI_SRCDIR), *.c)
CLI_HFILES = $(call rwildcard, $(CLI_SRCDIR), *.h)
CLI_OFILES = $(patsubst $(CLI_SRCDIR)/%.c, $(CLI_OBJDIR)/%.o, $(CLI_CFILES))

TST_CFILES = $(call rwildcard, $(TST_SRCDIR), *.c)
TST_HFILES = $(call rwildcard, $(TST_SRCDIR), *.h)
TST_OFILES = $(patsubst $(TST_SRCDIR)/%.c, $(TST_OBJDIR)/%.o, $(TST_CFILES))

ALL_CFILES = $(call rwildcard, $(SRCDIR), *.c)

# Useful abbreviations
LIB = $(OUTDIR)/$(LIB_FNAME)
CLI = $(OUTDIR)/$(CLI_FNAME)
TST = $(OUTDIR)/$(TST_FNAME)

# ===================== #
# === Rules =========== #
# ===================== #

.PHONY: report install clean all

all: $(LIB) $(CLI) $(TST)

$(OBJDIR)/%.o: $(SRCDIR)/%.c
	@ mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $^ -o $@

$(LIB): $(LIB_OFILES)
	@ echo $(AR) rcs $@ ...
	@ $(AR) rcs $@ $(LIB_OFILES)

$(CLI): $(CLI_OFILES) $(LIB)
	$(CC) $^ -o $@ $(LFLAGS)

$(TST): $(TST_OFILES) $(LIB)
	gcc $^ -o $@ $(LFLAGS)

#$(REPORTDIR)/%.gcov: src/%.c
#	@ mkdir -p $(@D)
#	gcov --stdout $(patsubst $(SRCDIR)/%.c, $(OBJDIR)/%.c, $^) > $@
#
#report: $(patsubst $(SRCDIR)/%.c, $(REPORTDIR)/%.gcov, $(ALL_CFILES))
#	lcov --capture --directory $(OBJDIR) --output-file coverage.info
#	genhtml coverage.info --output-directory .
#

report: 
	lcov --capture --directory $(OBJDIR) --output-file $(OBJDIR)/coverage.info
	@ mkdir -p report
	genhtml $(OBJDIR)/coverage.info --output-directory report

install: $(LIB) $(CLI)
	sudo cp $(LIB) $(LIB_INSTALLDIR)
	sudo cp $(CLI) $(CLI_INSTALLDIR)

clean:
	rm -rf $(OBJDIR)
	rm -rf $(REPORTDIR)
	rm  -f $(LIB)
	rm  -f $(CLI)