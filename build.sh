#!/bin/bash

emcc -O3 -o src/demodulator_wasm.js -s EXPORTED_RUNTIME_METHODS=['ccall'] src/*.cc