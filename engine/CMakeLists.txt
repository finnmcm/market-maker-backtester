cmake_minimum_required(VERSION 3.10)
project(MyProject)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Find Boost libraries (system, filesystem, thread)
find_package(Boost 1.71 REQUIRED COMPONENTS system filesystem thread)

# Find OpenSSL for TLS
find_package(OpenSSL REQUIRED)


# Find Threads (for portability)
find_package(Threads REQUIRED)

# Add source files
add_executable(ordermatch main.cpp orderbook.cpp tcp_reader.cpp websocket_reader.cpp)

# Include directories
target_include_directories(ordermatch
    PRIVATE
        ${Boost_INCLUDE_DIRS}
        ${OPENSSL_INCLUDE_DIR}
        ${nlohmann_json_INCLUDE_DIRS}
)

# Link libraries
# Use Boost variables to ensure compatibility
target_link_libraries(ordermatch
    PRIVATE
        ${Boost_SYSTEM_LIBRARY}
        ${Boost_FILESYSTEM_LIBRARY}
        ${Boost_THREAD_LIBRARY}
        ${OPENSSL_SSL_LIBRARY}
        ${OPENSSL_CRYPTO_LIBRARY}
        pthread          # on Linux/macOS
)
