# Aux functions
rwildcard = $(foreach d, $(wildcard $(1:=/*)), $(call rwildcard ,$d, $2) $(filter $(subst *, %, $2), $d))

# Compiler
CXX = gcc
CC  = $(CROSS_COMPILE)$(CXX)

# Compiler flags
CFLAGS = -O3 -Wall -Wextra -g
LFLAGS = -lm

# Files and directories
SRCDIR  = src
OBJDIR  = temp
BINDIR  = build
OUTFILE = noja

# Find all files
SRC  = $(call rwildcard, $(SRCDIR), *.c)
OBJS = $(patsubst $(SRCDIR)/%.c, $(OBJDIR)/%.o, $(SRC))


# Compile everything to object files
$(OBJDIR)/%.o: $(SRCDIR)/%.c
	@ echo $(shell tput setaf 3) [C] $(shell tput setaf 5) [SRC] $(shell tput setaf 7) $^
	@ mkdir -p $(@D)
	@ $(CC) $(CFLAGS) -c $^ -o $@


# Clean all artifacts and rebuild the whole thing
all: clean $(OBJS) build

# Run fuzz
.PHONY fuzz: all
fuzz: 
	@ export AFL_I_DONT_CARE_ABOUT_MISSING_CRASHES=1
	@ export AFL_SKIP_CPUFREQ=1
	afl-fuzz -i examples/ -o out -m none -d -- ./build/noja run @@

# Link all object files
build:
	@ echo !==== LINKING
	@ mkdir -p $(BINDIR)
	@ $(CC) -o $(BINDIR)/$(OUTFILE) $(OBJS) $(LFLAGS)

clean:
	@ rm -rf $(BINDIR)
	@ rm -rf $(OBJDIR)
