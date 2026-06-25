# from repo root: market-maker-simulator/
cmake --preset debug && cmake --build --preset debug --target sim
./build/debug/sim/sim --seed 42 --ticks 8
(--target sim just skips rebuilding ordermatch; drop it to build everything. release/asan/tsan presets work the same way.)
