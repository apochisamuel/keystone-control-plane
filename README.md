# Aegis Control Plane

Aegis Control Plane is an offline C++20 library for reading, validating,
replaying, and inspecting compact control-plane capture streams. It models
session establishment, subscriber registration, bearer and tunnel state,
policy staging, routing, handover, replay control, audit logging, snapshotting,
and recovery.

The binary capture format is frame-oriented. A frame carries an operation, a
session identifier, and a small TLV payload. The engine is designed for batch
inspection tools and deterministic replay services rather than live network
transport. No runtime network access, credentials, or package downloads are
required.

## Build

```bash
cmake -S . -B build -DAEGIS_BUILD_TESTS=ON -DAEGIS_BUILD_TOOLS=ON
cmake --build build -j
ctest --test-dir build --output-on-failure
```

Fuzz targets are optional in the CMake build because libFuzzer is normally
provided by the fuzzing environment. ClusterFuzzLite builds the four harnesses
through `.clusterfuzzlite/build.sh`.

## Components

- Binary cursor, writer, varint, checksum, TLV, and text utilities.
- Capture framing, reassembly, extension blocks, and trace decoding.
- Subscriber, session, bearer, tunnel, replay, route, and dispatch state.
- Policy parsing, compilation, transactions, journaling, snapshots, recovery planning, and replay.
- Routing constraints, capacity scheduling, traffic shaping, quota forecasting, and service topology reports.
- Capture archives, diagnostics pipelines, versioned snapshots, deterministic scenario simulation, and metrics helpers.

See `docs/ARCHITECTURE.md` and `docs/CAPTURE_FORMAT.md` for details.
