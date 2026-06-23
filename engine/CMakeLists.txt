# engine/ — limit order book matching engine.
#
# Builds two things:
#   engine_lib  — STATIC library with the book + TCP wire; everything downstream
#                 (sim, flow, mm, bench, tests) links this for the in-process path.
#   ordermatch  — standalone executable running the TCP order-entry wire.
#
# The Coinbase WebSocket feed is off by default (it is unused by the simulator and
# pulls in OpenSSL + Boost.Beast). Enable with -DBUILD_WS=ON if you want it.

option(BUILD_WS "Build the Coinbase WebSocket feed (needs OpenSSL + Boost.Beast)" OFF)

find_package(Boost 1.71 REQUIRED COMPONENTS system thread)
find_package(Threads REQUIRED)

set(ENGINE_SOURCES
    orderbook.cpp
    tcp_reader.cpp
)
if(BUILD_WS)
    list(APPEND ENGINE_SOURCES websocket_reader.cpp)
endif()

add_library(engine_lib STATIC ${ENGINE_SOURCES})

# PUBLIC include dir so downstream targets (and the exe) resolve engine headers.
target_include_directories(engine_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# Boost.Lockfree / Boost.Asio are header-only; Boost::system carries their includes.
target_link_libraries(engine_lib
    PUBLIC
        Boost::system
        Boost::thread
        Threads::Threads
)

if(BUILD_WS)
    find_package(OpenSSL REQUIRED)
    target_compile_definitions(engine_lib PUBLIC ENABLE_WS)
    target_link_libraries(engine_lib PUBLIC OpenSSL::SSL OpenSSL::Crypto)
endif()

add_executable(ordermatch main.cpp)
target_link_libraries(ordermatch PRIVATE engine_lib)
