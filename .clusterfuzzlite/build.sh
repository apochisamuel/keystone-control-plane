#!/bin/bash -eu
cd $SRC

CXX=${CXX:-clang++}
AR=${AR:-llvm-ar}
mkdir -p "$OUT/obj"

objects=""
for src in src/*.cpp; do
    obj="$OUT/obj/$(basename "${src%.cpp}").o"
    "$CXX" ${CXXFLAGS:-} -std=c++20 -Isrc -Iinclude -c -o "$obj" "$src"
    objects="$objects $obj"
done

"$AR" rcs "$OUT/libaegis_control_plane.a" $objects

for fuzzer_src in fuzz/*_fuzzer.cc; do
    fuzzer_name=$(basename "${fuzzer_src%.cc}")
    "$CXX" ${CXXFLAGS:-} -std=c++20 -Isrc -Iinclude -o "$OUT/$fuzzer_name" \
        "$fuzzer_src" "$OUT/libaegis_control_plane.a" ${LIB_FUZZING_ENGINE:-} 
done
