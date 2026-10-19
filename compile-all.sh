#!/usr/bin/bash
set -euo pipefail

cmake -S . -B build
cmake --build build --parallel

cmake -S . -B build-release -DRAIJIN_ENABLE_SDL3=ON -DCMAKE_BUILD_TYPE=Release
cmake --build build-release --parallel

cmake -S . -B build-headless -DRAIJIN_ENABLE_SDL3=OFF
cmake --build build-headless --parallel

cmake \
    -S examples/starter \
    -B build-starter \
    -DRAIJIN_ROOT="$PWD" \
    -DRAIJIN_BUILD_DIR="$PWD/build-release" \
    -DCMAKE_BUILD_TYPE=Release
cmake --build build-starter --parallel
