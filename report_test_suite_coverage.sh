make -B BUILD_MODE=COVERAGE CC=gcc
./test tests/compiler/ tests/runtime/
make report
firefox report/index.html