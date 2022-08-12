
all: tester

libnoja.a:
	make -B -C .. tests/libnoja.a BUILD_MODE=RELEASE OUTDIR=tests

tester: tester.c libnoja.a
	gcc $^ -o $@ -Wall -Wextra -lm -g