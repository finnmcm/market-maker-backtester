# sim/ — in-process backtest harness / clock / wiring.
# Single-threaded and deterministic: drives the engine by direct calls (no TCP,
# no threads). Links engine_lib for the in-process path.

add_executable(sim sim.cpp)
target_link_libraries(sim PRIVATE engine_lib)
# sim/ for flow_generator.h; sim/flow/ for the ZI flow unit #included by sim.cpp.
target_include_directories(sim PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/flow)
