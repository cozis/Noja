./build.sh --fuzzing
export AFL_I_DONT_CARE_ABOUT_MISSING_CRASHES=1
export AFL_SKIP_CPUFREQ=1
afl-fuzz -i examples/ -o out -m none -d -- ./build/noja run @@