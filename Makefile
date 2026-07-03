.PHONY: all build test clean fuzz

all: build

build:
	cmake -S . -B build -DAEGIS_BUILD_TESTS=ON -DAEGIS_BUILD_TOOLS=ON
	cmake --build build -j

test: build
	ctest --test-dir build --output-on-failure

fuzz:
	cmake -S . -B build-fuzz -DAEGIS_BUILD_FUZZERS=ON -DAEGIS_ENABLE_SANITIZERS=ON
	cmake --build build-fuzz -j

clean:
	rm -rf build build-fuzz
