# Development

The normal CMake build compiles every production source file. The fuzzing build
uses the same code and links four independent harnesses. Keep protocol changes
backwards-compatible where practical and add focused tests for new transitions.

Development tooling is intentionally dependency-free. The command-line tools
are examples for inspection, example generation, and policy validation.
