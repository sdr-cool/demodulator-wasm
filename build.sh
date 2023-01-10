#!/bin/bash

emcc -O3 -o src/demodulator_wasm.mjs -s WASM=1 -s BINARYEN_ASYNC_COMPILATION=0 -s SINGLE_FILE=1 -s EXPORTED_RUNTIME_METHODS=['ccall'] src/*.cc