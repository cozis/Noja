FILES="tests/assembler_test.c    \
       src/assembler/assembler.c \
       src/common/executable.c   \
       src/utils/error.c         \
       src/utils/source.c        \
       src/utils/bpalloc.c       \
       src/utils/promise.c       \
       src/utils/labellist.c     \
       src/utils/bucketlist.c"

gcc $FILES -o build/assembler_test_cov -Wall -Wextra -g --coverage
gcc $FILES -o build/assembler_test -Wall -Wextra -g

