# Top-level build for the market-making simulator.
# CMakePresets.json attaches here (debug / asan / tsan / release).
# Each component is its own subdirectory; add them as they land.
cmake_minimum_required(VERSION 3.21)

# Boost >= 1.70 ships its own BoostConfig.cmake; prefer it over CMake's removed
# FindBoost module (silences the CMP0167 dev-warning on CMake >= 3.30).
if(POLICY CMP0167)
  cmake_policy(SET CMP0167 NEW)
endif()

project(market_maker_simulator CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Default to Debug for plain `cmake -B build` (single-config generators).
# Presets override this explicitly.
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Debug CACHE STRING "" FORCE)
endif()

# The matching engine: exposes engine_lib (linked by everything downstream)
# plus the standalone `ordermatch` TCP executable.
add_subdirectory(engine)

# Components below are added as the build plan reaches them:
# add_subdirectory(sim)     # simulation harness / clock / wiring
# add_subdirectory(flow)    # synthetic flow generator
# add_subdirectory(mm)      # market-maker strategy
# add_subdirectory(bench)   # microbenchmarks / latency harnesses
# add_subdirectory(tests)   # unit + scenario tests
