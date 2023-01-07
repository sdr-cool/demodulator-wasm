#!/bin/bash

emcc -O3 -o src/demodulator_wasm.js -s EXPORTED_RUNTIME_METHODS=['ccall'] \
  src/*.cc \
  -I liquid/include \
  liquid/src/libliquid.c \
  liquid/src/math/src/windows.c \
  liquid/src/math/src/math.c \
  liquid/src/math/src/math.*.c \
  liquid/src/utility/src/utility.c \
  liquid/src/utility/src/msb_index.c \
  liquid/src/filter/src/firdes.c \
  liquid/src/filter/src/filter_rrrf.c \
  liquid/src/buffer/src/bufferf.c \
  liquid/src/dotprod/src/dotprod_rrrf.c \
  liquid/src/modem/src/modemcf.c
