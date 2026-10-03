# CLAUDE.md

Election poll micro-simulator in C++20: generates a synthetic voter population, runs repeated polls with different sampling and estimation methods, and summarizes bias / margin of error / coverage.

## Build & run

```bash
cmake -S . -B build && cmake --build build
./build/poll_sim          # ~23 s in Release; output is deterministic (fixed seeds)
```

No test suite yet. Verify changes by building with zero warnings and checking the printed tables.

## Layout

- `include/poll_sim/*.hpp`, `src/*.cpp`: library `poll_sim_core`; `src/main.cpp`: executable `poll_sim`.
- New `.cpp` files must be added to `poll_sim_core` in `CMakeLists.txt`.
- Pipeline: `params` → `population` → `sampling` / `experiment` → `poll` (tally) → `estimate` → `stats` → printed by `main.cpp`.
- Module responsibilities and formulas: [docs/design.md](docs/design.md).

## Conventions

- **Naive first, no premature optimization.** The user wants to optimize memory/performance later themselves to feel the difference. Do not add `reserve`, `uint8_t` enums, bit-packing, SoA, or threading unless asked.
- Code comments in English; docs (`README.md`, `docs/`) in Traditional Chinese.
- Everything lives in `namespace poll_sim`. Enums are contiguous from 0 and are used directly as array indices; each has a `k*Count` constant and `to_string`.
- All tunable parameters live in `include/poll_sim/params.hpp`. Invalid input throws `std::invalid_argument`.
- Population and sampling use separate seeds (`PopulationParams::seed`, `SimParams::seed`) so runs are reproducible.
- True values are computed from the generated population, not from parameters.

## Docs to keep in sync

- `docs/experiments.md` quotes actual output numbers. If parameters or simulation logic change, rerun and update them.
- `docs/design.md` describes methods and module layout; update when adding modules or methods.
- `docs/roadmap.md` lists planned work; next step is the MNAR non-response scenario.
