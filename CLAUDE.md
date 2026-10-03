# CLAUDE.md

Election poll micro-simulator in C++20: generates a synthetic voter population, runs repeated polls with different sampling and estimation methods, and summarizes bias / margin of error / coverage.

## Build & run

```bash
cmake --preset release && cmake --build --preset release
./build/poll_sim          # ~25 s in Release; output is deterministic (fixed seeds)
./build/poll_sim --order=sequential   # ~7 s; statistically equivalent, sequential memory access
```

Presets: `release` → `build/`, `profile` → `build-prof/` (Release flags + `-g -fno-omit-frame-pointer`, for `perf record -g`), `debug` → `build-debug/`. Without presets, pass `-DCMAKE_BUILD_TYPE=Profile` etc.; default is Release.

No test suite yet. Verify changes by building with zero warnings and checking the printed tables.

## Layout

- `include/poll_sim/*.hpp`, `src/*.cpp`: library `poll_sim_core`; `src/main.cpp`: executable `poll_sim`.
- New `.cpp` files must be added to `poll_sim_core` in `CMakeLists.txt`.
- Pipeline: `params` → `population` → `sampling` / `experiment` → `poll` (tally) → `estimate` → `stats` → printed by `main.cpp`.
- Module responsibilities and formulas: [docs/design.md](docs/design.md).

## Conventions

- **Naive first, no premature optimization.** The user wants to optimize memory/performance later themselves to feel the difference. Do not add `reserve`, bit-packing, SoA, or threading unless asked. (Done at the user's request so far: `std::uint8_t` enums, 32-bit frame indices, `reserve` for the phone frames, `--order=sequential`, bitmap instead of hash sets for random draws, duplicates counted by sorting after each poll, `FrameEntry` packing call attributes into the frame, one frame per sampling method shared across sample sizes.)
- Code comments in English; docs (`README.md`, `docs/`) in Traditional Chinese.
- Everything lives in `namespace poll_sim`. Enums are contiguous from 0 and are used directly as array indices; each has a `k*Count` constant and `to_string`.
- All tunable parameters live in `include/poll_sim/params.hpp`. Invalid input throws `std::invalid_argument`.
- Population and sampling use separate seeds (`PopulationParams::seed`, `SimParams::seed`) so runs are reproducible.
- True values are computed from the generated population, not from parameters.

## Docs to keep in sync

- `docs/design.md` describes methods and module layout; update when adding modules or methods.
- `docs/concepts.md` introduces sampling/weighting methods and terminology for non-specialists; update when adding a method or a new concept. No output numbers in docs.
- `docs/roadmap.md` lists planned work; next step is turnout (voters vs. election result as two true values).
