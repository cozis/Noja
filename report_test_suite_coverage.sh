make -B BUILD_MODE=COVERAGE CC=gcc
./test tests/suite
make report
firefox report/index.html