./build.sh --coverage
mkdir temp
export LLVM_PROFILE_FILE="temp/noja.profraw"
./build/noja run examples/json.noja
llvm-profdata merge -sparse temp/noja.profraw -o temp/noja.profdata
llvm-cov show ./build/noja -instr-profile=temp/noja.profdata >> build/coverage.txt
llvm-cov export ./build/noja -instr-profile=temp/noja.profdata >> build/coverage.json
rm -rf temp