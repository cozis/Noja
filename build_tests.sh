FILES="tests/assembler_test.c    \
       src/noja/assembler/assembler.c \
       src/noja/common/executable.c   \
       src/noja/utils/error.c         \
       src/noja/utils/source.c        \
       src/noja/utils/bpalloc.c       \
       src/noja/utils/promise.c       \
       src/noja/utils/labellist.c     \
       src/noja/utils/bucketlist.c"

gcc $FILES -o assembler_test_cov -Wall -Wextra -g --coverage
gcc $FILES -o assembler_test     -Wall -Wextra -g
