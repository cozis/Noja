
CC=gcc
FLAGS="-Wall -Wextra"

for arg in "$@"
do
	case $arg in
		   --debug) FLAGS="$FLAGS -DDEBUG -g" 
			  	    ;;
		 --release) FLAGS="$FLAGS -DNDEBUG -O3" 
			  	    ;;

		--valgrind) FLAGS="$FLAGS -DUSING_VALGRIND=1"
					;;

		--coverage) CC=clang
					FLAGS="$FLAGS -fprofile-instr-generate -fcoverage-mapping"
					;;

		 --fuzzing) CC=afl-clang-fast
					;;
	esac
done

mkdir temp

mkdir temp/utils
$CC -c src/utils/utf8.c       -o temp/utils/utf8.o       $FLAGS
$CC -c src/utils/hash.c       -o temp/utils/hash.o       $FLAGS
$CC -c src/utils/stack.c      -o temp/utils/stack.o      $FLAGS
$CC -c src/utils/error.c      -o temp/utils/error.o      $FLAGS
$CC -c src/utils/source.c     -o temp/utils/source.o     $FLAGS
$CC -c src/utils/bpalloc.c    -o temp/utils/bpalloc.o    $FLAGS
$CC -c src/utils/promise.c    -o temp/utils/promise.o    $FLAGS
$CC -c src/utils/bucketlist.c -o temp/utils/bucketlist.o $FLAGS

mkdir temp/objects
$CC -c src/objects/heap.c      -o temp/objects/heap.o      $FLAGS
$CC -c src/objects/o_int.c     -o temp/objects/o_int.o     $FLAGS
$CC -c src/objects/o_map.c     -o temp/objects/o_map.o     $FLAGS
$CC -c src/objects/o_list.c    -o temp/objects/o_list.o    $FLAGS
$CC -c src/objects/o_none.c    -o temp/objects/o_none.o    $FLAGS
$CC -c src/objects/o_bool.c    -o temp/objects/o_bool.o    $FLAGS
$CC -c src/objects/o_file.c    -o temp/objects/o_file.o    $FLAGS
$CC -c src/objects/o_dir.c     -o temp/objects/o_dir.o     $FLAGS
$CC -c src/objects/o_float.c   -o temp/objects/o_float.o   $FLAGS
$CC -c src/objects/o_string.c  -o temp/objects/o_string.o  $FLAGS
$CC -c src/objects/o_buffer.c  -o temp/objects/o_buffer.o  $FLAGS
$CC -c src/objects/o_closure.c -o temp/objects/o_closure.o $FLAGS
$CC -c src/objects/objects.c   -o temp/objects/objects.o   $FLAGS

mkdir temp/compiler
$CC -c src/compiler/parse.c      -o temp/compiler/parse.o      $FLAGS
$CC -c src/compiler/compile.c    -o temp/compiler/compile.o    $FLAGS

mkdir temp/common
$CC -c src/common/executable.c -o temp/common/executable.o $FLAGS

mkdir temp/runtime
$CC -c src/runtime/runtime_error.c -o temp/runtime/runtime_error.o $FLAGS
$CC -c src/runtime/runtime.c 	   -o temp/runtime/runtime.o       $FLAGS
$CC -c src/runtime/o_nfunc.c       -o temp/runtime/o_nfunc.o       $FLAGS
$CC -c src/runtime/o_func.c        -o temp/runtime/o_func.o        $FLAGS
$CC -c src/runtime/o_staticmap.c   -o temp/runtime/o_staticmap.o   $FLAGS

mkdir temp/builtins
$CC -c src/builtins/basic.c -o temp/builtins/basic.o $FLAGS
$CC -c src/builtins/files.c -o temp/builtins/files.o $FLAGS
$CC -c src/builtins/math.c  -o temp/builtins/math.o  $FLAGS

rm -rf build
mkdir build

ar rcs build/libnoja-objects.a \
	temp/objects/heap.o    \
	temp/objects/o_int.o   \
	temp/objects/o_float.o \
	temp/objects/objects.o \
	temp/utils/error.o

ar rcs build/libnoja-compile.a \
	temp/compiler/parse.o   \
	temp/compiler/compile.o \
	temp/utils/bpalloc.o    \
	temp/utils/error.o      \
	temp/utils/source.o

ar rcs build/libnoja-runtime.a \
	temp/runtime/runtime.o  \
	temp/runtime/runtime_error.o \
	temp/runtime/o_func.o   \
	build/libnoja-compile.a \
	build/libnoja-objects.a

$CC src/main.c \
	temp/utils/utf8.o        \
	temp/utils/hash.o        \
	temp/utils/stack.o       \
	temp/utils/source.o      \
	temp/utils/promise.o     \
	temp/utils/bucketlist.o  \
	temp/objects/o_map.o     \
	temp/objects/o_none.o    \
	temp/objects/o_list.o    \
	temp/objects/o_file.o    \
	temp/objects/o_dir.o     \
	temp/objects/o_bool.o    \
	temp/objects/o_buffer.o  \
	temp/objects/o_string.o  \
	temp/objects/o_closure.o \
	temp/runtime/runtime.o 	 \
	temp/runtime/runtime_error.o \
	temp/runtime/o_nfunc.o   \
	temp/runtime/o_func.o    \
	temp/runtime/o_staticmap.o \
	temp/builtins/basic.o    \
	temp/builtins/files.o    \
	temp/builtins/math.o     \
	temp/common/executable.o \
	-o build/noja $FLAGS -Lbuild/ -lnoja-compile -lnoja-objects -lm

rm -rf temp
