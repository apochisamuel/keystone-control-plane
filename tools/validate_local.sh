#!/bin/bash -eu
cmake -S . -B build -DAEGIS_BUILD_TESTS=ON -DAEGIS_BUILD_TOOLS=ON
cmake --build build -j
ctest --test-dir build --output-on-failure
./tools/preflight.sh
