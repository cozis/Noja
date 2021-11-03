USING_VALGRIND=0

FLAGS="-L3p/libs/ -I3p/include/ -Wall -Wextra -g -DUSING_VALGRIND=$USING_VALGRIND"

mkdir temp

mkdir temp/utils
gcc -c src/utils/hash.c       -o temp/utils/hash.o       $FLAGS
gcc -c src/utils/stack.c      -o temp/utils/stack.o      $FLAGS
gcc -c src/utils/error.c      -o temp/utils/error.o      $FLAGS
gcc -c src/utils/source.c     -o temp/utils/source.o     $FLAGS
gcc -c src/utils/bpalloc.c    -o temp/utils/bpalloc.o    $FLAGS
gcc -c src/utils/promise.c    -o temp/utils/promise.o    $FLAGS
gcc -c src/utils/bucketlist.c -o temp/utils/bucketlist.o $FLAGS

mkdir temp/objects
gcc -c src/objects/heap.c     -o temp/objects/heap.o     $FLAGS
gcc -c src/objects/o_int.c    -o temp/objects/o_int.o    $FLAGS
gcc -c src/objects/o_map.c    -o temp/objects/o_map.o    $FLAGS
gcc -c src/objects/o_list.c   -o temp/objects/o_list.o   $FLAGS
gcc -c src/objects/o_none.c   -o temp/objects/o_none.o   $FLAGS
gcc -c src/objects/o_bool.c   -o temp/objects/o_bool.o   $FLAGS
gcc -c src/objects/o_float.c  -o temp/objects/o_float.o  $FLAGS
gcc -c src/objects/o_string.c -o temp/objects/o_string.o $FLAGS
gcc -c src/objects/objects.c  -o temp/objects/objects.o  $FLAGS

mkdir temp/compiler
gcc -c src/compiler/parse.c      -o temp/compiler/parse.o      $FLAGS
gcc -c src/compiler/compile.c    -o temp/compiler/compile.o    $FLAGS
gcc -c src/compiler/serialize.c  -o temp/compiler/serialize.o  $FLAGS

mkdir temp/common
gcc -c src/common/executable.c -o temp/common/executable.o $FLAGS

mkdir temp/runtime
gcc -c src/runtime/runtime_error.c -o temp/runtime/runtime_error.o $FLAGS
gcc -c src/runtime/runtime.c 	   -o temp/runtime/runtime.o       $FLAGS
gcc -c src/runtime/builtins.c 	   -o temp/runtime/builtins.o      $FLAGS
gcc -c src/runtime/o_nfunc.c       -o temp/runtime/o_nfunc.o       $FLAGS
gcc -c src/runtime/o_func.c        -o temp/runtime/o_func.o        $FLAGS

rm -rf build
mkdir build

ar rcs build/libnoja-objects.a \
	temp/objects/heap.o \
	temp/objects/o_int.o \
	temp/objects/o_float.o \
	temp/objects/objects.o \
	temp/utils/error.o

ar rcs build/libnoja-compile.a \
	temp/compiler/parse.o \
	temp/compiler/compile.o \
	temp/compiler/serialize.o \
	temp/utils/bpalloc.o \
	temp/utils/error.o \
	temp/utils/source.o

ar rcs build/libnoja-runtime.a \
	temp/runtime/runtime.o \
	temp/runtime/runtime_error.o \
	temp/runtime/o_func.o \
	build/libnoja-compile.a \
	build/libnoja-objects.a

gcc tests/src/test-parse.c   -o build/test-parse   $FLAGS -Lbuild/ -lnoja-compile -lxjson
gcc tests/src/test-objects.c -o build/test-objects $FLAGS -Lbuild/ -lnoja-objects

gcc src/main.c src/debug.c \
	temp/utils/hash.o \
	temp/utils/stack.o \
	temp/utils/source.o \
	temp/utils/promise.o \
	temp/utils/bucketlist.o \
	temp/objects/o_map.o \
	temp/objects/o_none.o \
	temp/objects/o_list.o \
	temp/objects/o_bool.o \
	temp/objects/o_string.o \
	temp/runtime/runtime.o \
	temp/runtime/runtime_error.o \
	temp/runtime/builtins.o \
	temp/runtime/o_nfunc.o \
	temp/runtime/o_func.o \
	temp/common/executable.o \
	-o build/noja $FLAGS -Lbuild/ -lnoja-compile -lnoja-objects -lxjson

rm -rf temp