
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

# Source directory for each program to be build
# They are relative to the SRCDIR folder.
LIB_SUBDIR = noja
CLI_SUBDIR = cli

# Resulting file names
LIB_FNAME = libnoja.a
CLI_FNAME = noja

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

# ===================== #
# === Variables ======= #
# ===================== #

# Auxiliary function
rwildcard = $(foreach d, $(wildcard $(1:=/*)), $(call rwildcard ,$d, $2) $(filter $(subst *, %, $2), $d))

# Absolute path of each program's source tree
LIB_SRCDIR = $(SRCDIR)/$(LIB_SUBDIR)
CLI_SRCDIR = $(SRCDIR)/$(CLI_SUBDIR)

# Each program that is being build uses as object
# cache a subfolder of OBJDIR called like the it's
# source tree base folder in SRCDIR.
LIB_OBJDIR = $(OBJDIR)/$(LIB_SUBDIR)
CLI_OBJDIR = $(OBJDIR)/$(CLI_SUBDIR)

LIB_CFILES = $(call rwildcard, $(LIB_SRCDIR), *.c)
LIB_HFILES = $(call rwildcard, $(LIB_SRCDIR), *.h)
LIB_OFILES = $(patsubst $(LIB_SRCDIR)/%.c, $(LIB_OBJDIR)/%.o, $(LIB_CFILES))

CLI_CFILES = $(call rwildcard, $(CLI_SRCDIR), *.c)
CLI_HFILES = $(call rwildcard, $(CLI_SRCDIR), *.h)
CLI_OFILES = $(patsubst $(CLI_SRCDIR)/%.c, $(CLI_OBJDIR)/%.o, $(CLI_CFILES))

# Useful abbreviations
LIB = $(OUTDIR)/$(LIB_FNAME)
CLI = $(OUTDIR)/$(CLI_FNAME)

# ===================== #
# === Rules =========== #
# ===================== #

all: $(LIB) $(CLI)

$(OBJDIR)/%.o: $(SRCDIR)/%.c
	@ mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $^ -o $@

$(LIB): $(LIB_OFILES)
	@ echo $(AR) rcs $@ ...
	@ $(AR) rcs $@ $(LIB_OFILES)

$(CLI): $(CLI_OFILES) $(LIB)
	$(CC) $^ -o $@ $(LFLAGS)

install: $(LIB) $(CLI)
	sudo cp $(LIB) $(LIB_INSTALLDIR)
	sudo cp $(CLI) $(CLI_INSTALLDIR)

clean:
	rm -rf $(OBJDIR)
	rm  -f $(LIB)
	rm  -f $(CLI)